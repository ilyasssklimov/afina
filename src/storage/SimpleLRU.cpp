#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::AddNode(const std::string &key, const std::string &value)
{
    
    
    return true;
}


bool DeleteNode(lru_map::iterator old_node)
{   
    if (*old_node == nullptr)
        return false;
    
    return true;
}


bool SimpleLRU::DeleteTailNode()
{
    if (_lru_tail == nullptr)
        return false;
    
    _cur_size -= (_lru_tail.get()->key.size() + _lru_tail_get()->value.size());
    
    // one element in list
    if (_lru_head == _lru_tail)
    {
        _lru_index.erase(_lru_tail.key);
        _lru_tail = nullptr;
        _lru_head = nullptr;
    }
    // more than one => change pointers
    else
    {
        lru_node *node = _lru_tail.get();
        _lru_tail = std::move(node->prev);
        _lru_tail.get()->next = nullptr;
        
        _lru_index.erase(node.key);
    }
    
    return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) 
{ 
    if (key.empty())
        return false;
    
    size_t adding_size = value.size();
    auto finding_node = _lru_index.find(key);
    
    // count size for new node
    if (finding_node != _lru_index.end())
    {
        size_t old_size = finding_node->second.get().value.size();
        if (adding_size > old_size)
            adding_size -= old_size;
    }
    
    bool is_deleted = false;  // check if finding node deleted
    // clean up memory for new node
    while (_cur_size + adding_size > _max_size)
    {
        if (_lru_tail.get()->key == key)
            is_deleted = true;
            
        if (DeleteTailNode() == false)
            return false;
    }
    
    if (is_deleted)
        DeleteNode(finding_node);
        
    return AddNode(key, value); 
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { return false; }

} // namespace Backend
} // namespace Afina
