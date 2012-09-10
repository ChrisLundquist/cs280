#include "ObjectAllocator.h"

#define MAX(a,b) a > b ? a : b

ObjectAllocator::ObjectAllocator(unsigned ObjectSize, const OAConfig& config) throw(OAException) {
    OAStats_ = OAStats();
    OAStats_.ObjectSize_ = ObjectSize;
    Config_ = config;
    OAStats_.PageSize_ = Config_.ObjectsPerPage_ * ObjectSize + sizeof(void*);
    used_objects = std::vector<void*>();
    free_objects = std::vector<void*>();
    new_page();
}

// Destroys the ObjectManager (never throws)
ObjectAllocator::~ObjectAllocator() throw() {
}

inline static void OAStatsAllocate(OAStats& stats) {
    stats.FreeObjects_--;
    stats.Allocations_++;
    stats.ObjectsInUse_++;
    stats.MostObjects_ = MAX(stats.MostObjects_, stats.ObjectsInUse_);
}

void ObjectAllocator::new_page() {
    char* allocation = new char[OAStats_.PageSize_];
    OAStats_.PagesInUse_++;
    OAStats_.FreeObjects_ += Config_.ObjectsPerPage_;

    for( unsigned i = 0; i < Config_.ObjectsPerPage_; i++) {
        char* object = allocation + i * OAStats_.ObjectSize_;
        free_objects.push_back(object);
    }
}

// Take an object from the free list and give it to the client (simulates new)
// Throws an exception if the object can't be allocated. (Memory allocation problem)
void *ObjectAllocator::Allocate() throw(OAException) {
    OAStatsAllocate(OAStats_);
    char* allocation = NULL;

    if(OAStats_.ObjectsInUse_ == Config_.MaxPages_ * Config_.ObjectsPerPage_)
        throw OAException(OAException::E_NO_MEMORY, "No more room sir");

    if(!Config_.UseCPPMemManager_) {
        // TODO
        if(free_objects.size() == 0)
            new_page();
        allocation = (char*)free_objects.back();
        free_objects.pop_back();
        used_objects.push_back(allocation);
    } else {
        allocation = new char[OAStats_.ObjectSize_];
    }
    used_objects.push_back(allocation);
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

    //if(! Object % Config_.Alignment_)
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

        free_objects.push_back(Object);
    }
    else
        delete[] (char*)Object;
}

// Calls the callback fn for each block still in use
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const {
    for( unsigned i = 0; i < used_objects.size(); i++)
        fn(used_objects[i],i);
    return 1;
}

// Calls the callback fn for each block that is potentially corrupted
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const {
    //TODO
    fn(NULL,0);
    return 1;
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
    return free_objects.front();
}

const void *ObjectAllocator::GetPageList() const {
    //TODO
    return NULL;
}

OAConfig ObjectAllocator::GetConfig() const {
    return Config_;
}

OAStats ObjectAllocator::GetStats() const {
    return OAStats_;
}
