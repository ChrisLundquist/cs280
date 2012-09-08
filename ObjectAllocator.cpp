#include "ObjectAllocator.h"

#define MAX(a,b) a > b ? a : b

ObjectAllocator::ObjectAllocator(unsigned ObjectSize, const OAConfig& config) throw(OAException) {
    OAStats_ = OAStats();
    OAStats_.ObjectSize_ = ObjectSize;
    Config_ = config;
    OAStats_.PageSize_ = Config_.ObjectsPerPage_ * ObjectSize + sizeof(void*);
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

// Take an object from the free list and give it to the client (simulates new)
// Throws an exception if the object can't be allocated. (Memory allocation problem)
void *ObjectAllocator::Allocate() throw(OAException) {
    OAStatsAllocate(OAStats_);

    if(Config_.UseCPPMemManager_) {
        // TODO
        return NULL;
    } else {
        return new char[OAStats_.ObjectSize_];
    }

}

inline static void OAStatsFree(OAStats& stats) {
    stats.FreeObjects_++;
    stats.Deallocations_++;
    stats.ObjectsInUse_--;
}

// Returns an object to the free list for the client (simulates delete)
// Throws an exception if the object can't be freed. (Invalid object)
void ObjectAllocator::Free(void *Object) throw(OAException) {
    OAStatsFree(OAStats_);

    if(Config_.UseCPPMemManager_){
        // TODO
    }
    else
        delete[] (char*)Object;

}

// Calls the callback fn for each block still in use
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const {
    //TODO
    fn(NULL,0);
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
    //TODO
    return NULL;
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
