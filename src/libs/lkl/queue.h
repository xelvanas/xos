#pragma once
#include <lkl.h>
#include <debug.h>

ns_lite_kernel_lib_begin

template<typename T> class queue_t;

template<typename T>
class qnode_t {

    friend class queue_t<T>;
private:

protected:
    // In order to prevent '_prev/_next' be modified from outside which 
    // can cause 'queue_t' failed to manage nodes, we put them in 'protected'
    // modifier and make 'queue_t' as a friend to limit access for outsiders.
    qnode_t* _prev = nullptr;
    qnode_t* _next = nullptr;
    T*       _pobj;
public:
    qnode_t() { }
    qnode_t(T* pobj) : _pobj(pobj) { }

    // since it's not a 'private/protected' member, we don't use prefix '_'
    

    qnode_t* prev() {
        return _prev;
    }

    const qnode_t* prev() const {
        return _prev;
    }

    qnode_t* next() {
        return _next;
    }

    const qnode_t* next() const {
        return _next;
    }

    bool alone() const {
        return _prev == nullptr &&
               _next == nullptr;
    }

    void leave() {
        _prev = nullptr;
        _next = nullptr;
    }

    T* get() const noexcept {
        return _pobj;
    }

    void reset(T* pobj) {
        _pobj = pobj;
    }

    explicit operator bool() const noexcept {
        return _pobj != nullptr;
    }

    T& operator*() const noexcept {
        ASSERT(_pobj != nullptr);
        return *_pobj;
    }

    T* operator->() const noexcept {
        ASSERT(_pobj != nullptr);
        return _pobj;
    }
};

template<typename T>
class queue_t {
public:
    typedef qnode_t<T> node;

private:
    node* _head = nullptr;
    node* _tail = nullptr;

public:
    node* head() {
        return _head;
    }

    const node* head() const {
        return _head;
    }

    node* tail() {
        return _tail;
    }

    const node* tail() const {
        return _tail;
    }

    bool unique() const {
        return _head != nullptr && _head == _tail;
    }

    bool empty() const {
        return _head == nullptr && _tail == _head;
    }

    void push_front(node* nd) 
    {
        if (nd == nullptr ||
            nd->alone() == false || // only alone node can push into queue
            (unique() && nd == _head)) {
            return;
        }

        if (empty()) {
            _head = nd;
            _tail = nd;
        }
        else {
            node* tmp = unique() ? _tail : _head;

            _head = nd;
            nd->_next = tmp;
            nd->_prev = nullptr;
            tmp->_prev = nd;
        }
    }

    node* pop_front()
    {
        if (empty()) {
            // no ASSERT yet, should throw exception or do something similar
            return nullptr;
        }
        else if (unique()) {
            // there's only one node to pop
            node* tmp = _head;
            _head = nullptr;
            _tail = nullptr;
            tmp->leave();
            return tmp;
        } else {
            node* tmp = _head;

            // only one node will be left
            if (_head->_next == _tail) {
                _head = _tail;
                _head->_next = nullptr;
                _head->_prev = nullptr;
            }
            else {
                _head = _head->_next;
                _head->_prev = nullptr;
            }
            tmp->leave();
            return tmp;
        }
    }

    void push_back(node* nd) 
    {
        if (nd == nullptr ||
            nd->alone() == false || // only alone node can push into queue
            (unique() && nd == _head)) {
            return;
        }

        if (empty()) {
            // no matter which way we push if queue_t is empty.
            push_front(nd);
        } else {
            node* tmp = unique() ? _head : _tail;
            _tail = nd;
            nd->_prev = tmp;
            nd->_next = nullptr;
            tmp->_next = nd;
        }
    }

    node* pop_back()
    {
        if (empty()) {
            return nullptr;
        }
        else if (unique()) {
            return pop_front();
        } else {
            node* tmp = _tail;
            if (_head->_next == _tail) {
                _tail = _head;
                _tail->_next = nullptr;
                _tail->_prev = nullptr;
            }
            else {
                _tail = _tail->_prev;
                _tail->_next = nullptr;
            }
            tmp->leave();
            return tmp;
        }
    }

    bool find(node* nd)
    {
        if (nd != nullptr)
        {
            // special case
            if (nd->alone()) {
                return unique() && nd == _head;
            }

            node* tmp = _head;
            while (tmp != nullptr) {
                if (nd == tmp) {
                    return true;
                }
                tmp = tmp->_next;
            }
        }
        return false;
    }

    void remove(node* nd) 
    {
        if (nd != nullptr &&
            empty() == false &&
            find(nd) == true)
        {
            if (nd == _head) {
                pop_front();
            }
            else if (nd == _tail) {
                pop_back();
            }
            else {
                nd->_prev->_next = nd->_next;
                nd->_next->_prev = nd->_prev;
                nd->leave();
            }
        }
    }

    // insert a new node before an exist node
    void insert(node* pos, node* nd) 
    {
        if (pos != nullptr &&
            nd  != nullptr &&
            nd  != pos     &&
            find(pos) == true) 
        {
            if (pos == _head) {
                push_front(nd);
            }
            else if (pos == _tail) {
                push_back(nd);
            }
            else {
                nd->_prev = pos->_prev;
                nd->_prev->_next = nd;
                nd->_next = pos;
                pos->_prev = nd;
            }
        }
    }

    uint32_t length() const 
    {
        uint32_t len = 0;
        auto itr = head();
        while (itr != nullptr)
        {
            ++len;
            itr = itr->next();
        };
        return len;
    }

};

ns_lite_kernel_lib_end