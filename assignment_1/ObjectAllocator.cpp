#include "ObjectAllocator.h"

#define MAX(a,b) a > b ? a : b

ObjectAllocator::ObjectAllocator(unsigned ObjectSize, const OAConfig& config) throw(OAException) {
    OAStats_ = OAStats();
    Config_ = config;

    OAStats_.ObjectSize_ = ObjectSize + Config_.HeaderBlocks_ + 2 * Config_.PadBytes_;
    OAStats_.PageSize_ =
        Config_.ObjectsPerPage_ * OAStats_.ObjectSize_ +
        sizeof(void*);

    used_objects = std::vector<void*>();
    free_objects = std::vector<void*>();
    pages = std::vector<void*>();

    new_page();
}

// Destroys the ObjectManager (never throws)
ObjectAllocator::~ObjectAllocator() throw() {
    for(unsigned i = 0; i < pages.size(); i++){
        delete[] (char*) pages[i];
    }
}

inline static void OAStatsAllocate(OAStats& stats) {
    stats.FreeObjects_--;
    stats.Allocations_++;
    stats.ObjectsInUse_++;
    stats.MostObjects_ = MAX(stats.MostObjects_, stats.ObjectsInUse_);
}

inline static char* safe_allocate(unsigned size) {
    try {
        return new char[size];
    }
    catch (std::bad_alloc &) {
        throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
    }
}

inline static void OAStatsNewPage(OAStats& stats, OAConfig& config) {
    stats.PagesInUse_++;
    stats.FreeObjects_ += config.ObjectsPerPage_;
}

void ObjectAllocator::new_page() {
    char* allocation = safe_allocate(OAStats_.PageSize_);
    if(Config_.DebugOn_) {
        memset(allocation, UNALLOCATED_PATTERN, OAStats_.PageSize_);
    }

    pages.push_back(allocation);

    OAStatsNewPage(OAStats_, Config_);

    for( unsigned i = 0; i < Config_.ObjectsPerPage_; i++) {
        char* object = allocation + i * OAStats_.ObjectSize_;
        free_objects.push_back(object);
    }
}

inline void ObjectAllocator::ValidateAllocate() const throw(OAException) {
    if(Config_.MaxPages_ > 0 && OAStats_.ObjectsInUse_ >= Config_.MaxPages_ * Config_.ObjectsPerPage_)
        throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No More Available Pages");

}

// Take an object from the free list and give it to the client (simulates new)
// Throws an exception if the object can't be allocated. (Memory allocation problem)
void *ObjectAllocator::Allocate() throw(OAException) {
    char* allocation = NULL;

    ValidateAllocate();
    OAStatsAllocate(OAStats_);

    if(!Config_.UseCPPMemManager_) {
        // TODO
        if(free_objects.size() == 0)
            new_page();
        allocation = (char*)free_objects.back();
        free_objects.pop_back();
        used_objects.push_back(allocation);
    } else {
        allocation = safe_allocate(OAStats_.ObjectSize_);
    }
    used_objects.push_back(allocation);

    if(Config_.DebugOn_) {
        memset(allocation, ALLOCATED_PATTERN, OAStats_.ObjectSize_);
    }
    return allocation;
}

inline static void OAStatsFree(OAStats& stats) {
    stats.FreeObjects_++;
    stats.Deallocations_++;
    stats.ObjectsInUse_--;
}

inline bool list_has( std::vector<void*> list, const void* object) {
    for(unsigned i = 0; i < list.size(); i++) {
        if(list[i] == object)
            return true;
    }
    return false;
}

inline int list_find( std::vector<void*> list, const void* object) {
    for(unsigned i = 0; i < list.size(); i++)
        if(list[i] == object)
            return i;
    return -1;
}

void ObjectAllocator::ValidateFree(const void *Object) const throw(OAException) {
    // Make sure the pointer hasn't been freed already
    if(list_has(free_objects, Object))
        throw OAException(OAException::E_MULTIPLE_FREE, "Multiple Free");

    // Make sure the pointer came from us
    if(! list_has(used_objects, Object))
        throw OAException(OAException::E_BAD_ADDRESS, "Bad Address");

    // FIXME
    //if(Config_.Alignment_ > 0 && Object % (void*)Config_.Alignment_ == 0)
    //   throw OAException(OAException::E_BAD_BOUNDARY, "Bad Boundary");
}

// Returns an object to the free list for the client (simulates delete)
// Throws an exception if the object can't be freed. (Invalid object)
void ObjectAllocator::Free(void *Object) throw(OAException) {
    // Throw any exceptions for invalid frees
    ValidateFree(Object);
    OAStatsFree(OAStats_);

    if(!Config_.UseCPPMemManager_){
        int index = list_find( used_objects, Object);
        used_objects.erase(used_objects.begin() + index);

        if(Config_.DebugOn_) {
            memset(Object, FREED_PATTERN, OAStats_.ObjectSize_);
        }

        free_objects.push_back(Object);
    }
    else
        delete[] (char*)Object;
}

// Calls the callback fn for each block still in use
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const {
    for( unsigned i = 0; i < used_objects.size(); i++)
        fn(used_objects[i], OAStats_.ObjectSize_);
    return 0;
}

// Calls the callback fn for each block that is potentially corrupted
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const {
    for( unsigned i = 0; i < used_objects.size(); i++)
        fn(used_objects[i], OAStats_.ObjectSize_);
    return 0;
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
    return NULL;
    //TODO
    return pages.front();
}

OAConfig ObjectAllocator::GetConfig() const {
    return Config_;
}

OAStats ObjectAllocator::GetStats() const {
    return OAStats_;
}
