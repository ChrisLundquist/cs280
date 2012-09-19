#include "ObjectAllocator.h"

#define MAX(a,b) a > b ? a : b
#define NOT_IN_USE 0
#define IN_USE 1

ObjectAllocator::ObjectAllocator(unsigned ObjectSize, const OAConfig& config) throw(OAException) {
    OAStats_ = OAStats();
    Config_ = config;

    OAStats_.ObjectSize_ = ObjectSize;
    OAStats_.PageSize_ =
        Config_.ObjectsPerPage_ * total_object_size() +
        sizeof(GenericObject*);

    used_objects = std::vector<unsigned char*>();
    free_objects = std::vector<unsigned char*>();
    pages = std::vector<unsigned char*>();

    new_page();
}

inline unsigned ObjectAllocator::total_object_size() const {
    return OAStats_.ObjectSize_ + Config_.HeaderBlocks_ + 2 * Config_.PadBytes_;
}

inline unsigned ObjectAllocator::object_size() const {
    return OAStats_.ObjectSize_;
}

// Destroys the ObjectManager (never throws)
ObjectAllocator::~ObjectAllocator() throw() {
    if(Config_.UseCPPMemManager_ == false)
        for(unsigned i = 0; i < pages.size(); i++) {
            delete[] pages[i];
        }
}

inline static void OAStatsAllocate(OAStats& stats) {
    stats.FreeObjects_--;
    stats.Allocations_++;
    stats.ObjectsInUse_++;
    stats.MostObjects_ = MAX(stats.MostObjects_, stats.ObjectsInUse_);
}

inline static unsigned char* safe_allocate(unsigned size) {
    try {
        return new unsigned char[size];
    } catch (std::bad_alloc &) {
        throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
    }
}

inline static void OAStatsNewPage(OAStats& stats, OAConfig& config) {
    stats.PagesInUse_++;
    stats.FreeObjects_ += config.ObjectsPerPage_;
}

void ObjectAllocator::new_page() {
    // Do nothing if our manager is turned off
    if(Config_.UseCPPMemManager_ == true)
        return;

    unsigned char* allocation = safe_allocate(OAStats_.PageSize_);

    pages.push_back(allocation);

    pages[OAStats_.PagesInUse_] = allocation;

    OAStatsNewPage(OAStats_, Config_);

    memset(allocation, 0, sizeof(GenericObject*));

    allocation += sizeof(GenericObject*);
    for( unsigned i = 0; i < Config_.ObjectsPerPage_; i++) {
        unsigned char* object = allocation_to_object(allocation + i * total_object_size());

        if(Config_.DebugOn_) { // And we are active?
            memset(object_to_allocation(object), UNALLOCATED_PATTERN, total_object_size());
            memset(object_to_left_pad(object),  PAD_PATTERN, Config_.PadBytes_);
            memset(object_to_right_pad(object), PAD_PATTERN, Config_.PadBytes_);
        }

        free_objects.push_back(object);
    }
}

inline unsigned char* ObjectAllocator::object_to_header(unsigned char* object) const {
    return object_to_left_pad(object) - Config_.HeaderBlocks_;
}

inline unsigned char* ObjectAllocator::object_to_left_pad(unsigned char* object) const{
    return object - Config_.PadBytes_;
}

inline unsigned char* ObjectAllocator::object_to_right_pad(unsigned char* object) const {
    return object + object_size();
}

inline unsigned char* ObjectAllocator::allocation_to_object(unsigned char* allocation) const {
    return allocation + Config_.PadBytes_ + Config_.HeaderBlocks_;
}

inline unsigned char* ObjectAllocator::object_to_allocation(unsigned char* object) const {
    return object - Config_.PadBytes_ - Config_.HeaderBlocks_;
}

inline void ObjectAllocator::ValidateAllocate() const throw(OAException) {
    if(Config_.UseCPPMemManager_ == false)
        if(Config_.MaxPages_ > 0 && OAStats_.ObjectsInUse_ >= Config_.MaxPages_ * Config_.ObjectsPerPage_)
            throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No More Available Pages");
}

inline void ObjectAllocator::MarkHeader(unsigned char* object, unsigned char flag) {
        if(Config_.HeaderBlocks_) {
            *(object_to_left_pad(object) - 1)  = flag;
        }
}

inline bool ObjectAllocator::CheckHeader(unsigned char* object, unsigned char flag) const {
        if(Config_.HeaderBlocks_) {
            return (*(object_to_left_pad(object) - 1)  == flag);
        }
        return false;
}

// Take an object from the free list and give it to the client (simulates new)
// Throws an exception if the object can't be allocated. (Memory allocation problem)
void *ObjectAllocator::Allocate() throw(OAException) {
    unsigned char* object = NULL;

    ValidateAllocate();
    OAStatsAllocate(OAStats_);

    if(Config_.UseCPPMemManager_ == false) {
        if(free_objects.size() == 0) // Need a new page
            new_page();

        object = free_objects.back();
        free_objects.pop_back();

        if(Config_.DebugOn_) {
            memset(object, ALLOCATED_PATTERN, OAStats_.ObjectSize_);
        }

        MarkHeader(object, IN_USE);

    } else {
        object = safe_allocate(total_object_size());
    }
    used_objects.push_back(object);

    return object;
}

inline static void OAStatsFree(OAStats& stats) {
    stats.FreeObjects_++;
    stats.Deallocations_++;
    stats.ObjectsInUse_--;
}

inline bool list_has( const std::vector<unsigned char*>& list, const unsigned char* object) {
    for(unsigned i = 0; i < list.size(); i++)
        if(list[i] == object)
            return true;
    return false;
}

inline int list_find( const std::vector<unsigned char*>& list, const unsigned char* object) {
    for(unsigned i = 0; i < list.size(); i++)
        if(list[i] == object)
            return i;
    return -1;
}

void ObjectAllocator::ValidateFree(unsigned char *Object) const throw(OAException) {
    if(Config_.UseCPPMemManager_)
        return;

    // Make sure the pointer hasn't been freed already
    if(CheckHeader(Object, IN_USE))
        throw OAException(OAException::E_MULTIPLE_FREE, "Multiple Free");
    else if(list_has(free_objects, Object))
        throw OAException(OAException::E_MULTIPLE_FREE, "Multiple Free");

    // Make sure the pointer came from us
    if(! list_has(used_objects, Object))
        throw OAException(OAException::E_BAD_ADDRESS, "Bad Address");

    // TODO Find what page the object is on
    //if( (Object - page) % total_object_size() != 0)
    //   throw OAException(OAException::E_BAD_BOUNDARY, "Bad Boundary");
}

inline bool ObjectAllocator::CorruptPadding(unsigned char* const object) const throw(OAException) {
    unsigned char* left_pad = object_to_left_pad(object);
    unsigned char* right_pad = object_to_right_pad(object);

    for( unsigned i = 0; i < Config_.PadBytes_; i++)
        if(*(left_pad + i) != PAD_PATTERN)
            return true;

    for( unsigned i = 0; i < Config_.PadBytes_; i++)
        if(*(right_pad + i) != PAD_PATTERN)
            return true;
    return false;
}

// Returns an object to the free list for the client (simulates delete)
// Throws an exception if the object can't be freed. (Invalid object)
void ObjectAllocator::Free(void *Object) throw(OAException) {
    unsigned char* char_object = reinterpret_cast<unsigned char*>(Object);

    // Throw any exceptions for invalid frees
    ValidateFree(char_object);
    OAStatsFree(OAStats_);

    if(Config_.UseCPPMemManager_ == false) {

        if(Config_.DebugOn_) {
            if(CorruptPadding(char_object))
                throw OAException(OAException::E_CORRUPTED_BLOCK, "Corrupt Block");

            memset(object_to_allocation(char_object), FREED_PATTERN, total_object_size());
        }

        MarkHeader(char_object, NOT_IN_USE);

        int index = list_find(used_objects, char_object);
        used_objects.erase(used_objects.begin() + index);

        free_objects.push_back(char_object);
    }
    else
        delete[] char_object;
}

// Calls the callback fn for each block still in use
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const {
    for( unsigned i = used_objects.size() - 1; i > 0; i--)
        fn(used_objects[i], OAStats_.ObjectSize_);
    return 0;
}

// Calls the callback fn for each block that is potentially corrupted
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const {
    unsigned bad = 0;

    for( unsigned i = 0; i < used_objects.size(); i++) {
        unsigned char* object = used_objects[i];
        if( CorruptPadding(object) ) {
            fn(object, OAStats_.ObjectSize_);
            bad++;
        }
    }

    return bad;
}

// Frees all empty pages
unsigned ObjectAllocator::FreeEmptyPages() {
    return 1;
}

// Returns true if FreeEmptyPages and alignments are implemented
bool ObjectAllocator::ImplementedExtraCredit() {
    return false;
}

// Testing/Debugging/Statistic methods
void ObjectAllocator::SetDebugState(bool State) {
    Config_.DebugOn_ = State;
}

const void *ObjectAllocator::GetFreeList() const {
    //TODO
    return free_objects.front();
}

const void *ObjectAllocator::GetPageList() const {
    //TODO
    return pages.front();
}

OAConfig ObjectAllocator::GetConfig() const {
    return Config_;
}

OAStats ObjectAllocator::GetStats() const {
    return OAStats_;
}
