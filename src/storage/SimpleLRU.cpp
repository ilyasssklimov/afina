#include <iostream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::AddNode(const std::string &key, const std::string &value)
{
    _cur_size += (key.size() + value.size());
    std::unique_ptr<lru_node> new_node (new lru_node{key, value, nullptr, nullptr});
    if (_lru_head)
        new_node->next = std::move(_lru_head);
    _lru_head = std::move(new_node);

    if (_lru_head->next)
        _lru_head->next->prev = _lru_head.get();
    else
        _lru_tail = _lru_head.get();

    _lru_index.emplace(_lru_head->key, *_lru_head);

    return true;
}


void SimpleLRU::UpdatePointers(lru_node *node)
{
    lru_node *prev = node->prev;
    lru_node *next = node->next.get();

    if (prev && next)
    {
        prev->next = std::move(node->next);
        next->prev = prev;
    }
    else if (prev && !next)
    {
        prev->next.reset(nullptr);
        _lru_tail = prev;
    }
    else if (!prev && next)
    {
        next->prev = nullptr;
        _lru_head = std::move(node->next);
    }
    else
    {
        _lru_tail = nullptr;
        _lru_head.reset(nullptr);
    }
}

bool SimpleLRU::DeleteNode(lru_node *old_node)
{
    if (old_node == nullptr)
        return false;

    _cur_size -= (old_node->key.size() + old_node->value.size());
    _lru_index.erase(old_node->key);
    UpdatePointers(old_node);

    return true;
}


bool SimpleLRU::CleanUpMemory(const std::string &key, const std::string &value, lru_node *node)
{
    size_t adding_size = value.size();

    // count size for new node
    if (node != nullptr)
    {
        size_t old_size = node->value.size();
        if (adding_size > old_size)
            adding_size -= old_size;
    }

    bool is_deleted = false;  // check if finding node deleted
    // clean up memory for new node
    while (_cur_size + adding_size > _max_size)
    {
        if (_lru_tail->key == key)
            is_deleted = true;

        if (!DeleteNode(_lru_tail))
            return false;
    }

    if (node && !is_deleted)
        DeleteNode(node);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{ 
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    lru_node *finding_node = nullptr;
    if (finding_it != _lru_index.end())
        finding_node = &finding_it->second.get();

    return CleanUpMemory(key, value, finding_node) && AddNode(key, value);
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it != _lru_index.end())
        return false;

    return CleanUpMemory(key, value, nullptr) && AddNode(key, value);
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it != _lru_index.end())
        return CleanUpMemory(key, value, &finding_it->second.get()) && AddNode(key, value);
    else
        return false;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    if (key.empty())
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it == _lru_index.end())
        return false;

    return DeleteNode(&finding_it->second.get());
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it != _lru_index.end())
    {
        lru_node *finding_node = &finding_it->second.get();
        if (finding_node->prev)
        {
            std::unique_ptr<lru_node> node = std::move(finding_node->prev->next);
            UpdatePointers(finding_node);
            _lru_head->prev = finding_node;
            
            finding_node->next = std::move(_lru_head);
            finding_node->prev = nullptr;
            _lru_head = std::move(node);
        }
        
        value = _lru_head->value;
        return true;
    }
    else
    {
        return false;
    }
}


} // namespace Backend
} // namespace Afina
