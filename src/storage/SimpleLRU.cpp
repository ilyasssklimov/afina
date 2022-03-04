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
    if (!node)
        return;

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


size_t SimpleLRU::CountSize(const std::string &key, const std::string &value, lru_node *node)
{
    size_t adding_size = value.size();

    if (!node)
    {
        adding_size += key.size();
    }
    else
    {
        size_t old_size = node->value.size();
        if (adding_size >= old_size)
             adding_size -= old_size;
    }

    return adding_size;
}


bool SimpleLRU::CleanUpMemory(const size_t adding_size)
{
    while (_cur_size + adding_size > _max_size)
    {
        if (!DeleteNode(_lru_tail))
            return false;
    }

    return true;
}


void SimpleLRU::MoveNodeToHead(std::unique_ptr<lru_node> &node)
{
    if (node == _lru_head)
        return;

    lru_node *finding_node = node.get();
    _lru_head->prev = finding_node;
    finding_node->next = std::move(_lru_head);
    finding_node->prev = nullptr;
    _lru_head = std::move(node);
}


bool SimpleLRU::SetNodeValue(lru_node *node, const std::string &value)
{
    if (!node)
        return false;

    _cur_size += value.size() - node->value.size();
    node->value = value;

    return true;
}


bool SimpleLRU::SetNode(const std::string &key, const std::string &value, lru_node *finding_node)
{
    size_t adding_size = CountSize(key, value, finding_node);
    std::unique_ptr<lru_node> node;
    if (finding_node && finding_node->prev)
        node = std::move(finding_node->prev->next);

    if (!finding_node)
    {
        return CleanUpMemory(adding_size) && AddNode(key, value);
    }
    else
    {
        if (finding_node->prev)
        {
            UpdatePointers(finding_node);
            MoveNodeToHead(node);
        }

        return CleanUpMemory(adding_size) && SetNodeValue(finding_node, value);
    }
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

    return SetNode(key, value, finding_node);
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it != _lru_index.end())
        return false;

    size_t adding_size = CountSize(key, value, nullptr);
    return CleanUpMemory(adding_size) && AddNode(key, value);
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    if (key.empty() || key.size() + value.size() > _max_size)
        return false;

    auto finding_it = _lru_index.find(key);
    if (finding_it == _lru_index.end())
        return false;

    lru_node *finding_node = &finding_it->second.get();
    return SetNode(key, value, finding_node);
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
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
    if (finding_it == _lru_index.end())
        return false;

    lru_node *finding_node = &finding_it->second.get();
    if (finding_node->prev)
    {
        std::unique_ptr<lru_node> node = std::move(finding_node->prev->next);
        UpdatePointers(finding_node);
        MoveNodeToHead(node);
    }
    value = _lru_head->value;
    return true;
}


} // namespace Backend
} // namespace Afina
