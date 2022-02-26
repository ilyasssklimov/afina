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
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _cur_size(0), _lru_head(nullptr), _lru_tail(nullptr) {}

    ~SimpleLRU() {
        _lru_index.clear();
        _lru_head.reset(); // TODO: Here is stack overflow
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
        std::unique_ptr <lru_node> prev;
        lru_node *next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size;
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness".
    // List owns all nodes
    
    lru_node *_lru_head;                  // new elements to head
    std::unique_ptr<lru_node> _lru_tail;  // old elements in tail
    
    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    using lru_map = std::map<std::reference_wrapper<std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>;
    lru_map _lru_index;
    
    // add new node to head
    bool AddNode(const std::string &key, const std::string &value);
    
    // delete node by key
    bool DeleteNode(const std::string &key);
    
    // delete node from tail
    bool DeleteTailNode();
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_SIMPLE_LRU_H
