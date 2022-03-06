#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>


namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    explicit SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _cur_size(0), _lru_tail(nullptr) {
        _lru_head.reset(nullptr);
    }

    ~SimpleLRU() {
        _lru_index.clear();
        while (_lru_tail)
            DeleteNode(_lru_tail);
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;

        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    // add new node to head
    bool AddNode(const std::string &key, const std::string &value);

    // update pointers while deleting node
    void UpdatePointers(lru_node *node);

    // delete node by pointer
    bool DeleteNode(lru_node *old_node);

    // count adding size for new node
    size_t CountDeltaSize(const std::string &key, const std::string &value, lru_node *node);

    // clean up memory if future size will be more than max size
    bool CleanUpMemory(const size_t adding_size);

    // move node to head
    void MoveNodeToHead(std::unique_ptr<lru_node> &node);

    // set value to node
    bool SetNodeValue(lru_node *node, const std::string &value);

    // set or add node with move to head
    bool SetNode(const std::string &key, const std::string &value, lru_node *finding_node);

private:
    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size;
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness".
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;  // new elements to head
    lru_node *_lru_tail;                  // old elements in tail
    
    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    using ref_string = std::reference_wrapper<const std::string>;
    using ref_lru_node = std::reference_wrapper<lru_node>;
    using comp_string = std::less<std::string>;
    std::map<ref_string, ref_lru_node, comp_string> _lru_index;
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_SIMPLE_LRU_H
