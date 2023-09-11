/*
    File: Queue.cpp
    Queue is a template class for a simple thread safe processing queue
    in a typical scenario with one producer and at least one consumer thread

    written by Elias Geiger
*/

#include <queue>
#include <mutex>

template<class T> class Queue {

    std::queue<T> m_queue;
    std::mutex m_mutex;

public:
    // Pushes new element into queue.
    // function blocks as long as mutex is locked by someone else
    void push(T elem) 
    {
        // apply lock guard for thread safe access to the queue
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(elem);
    }

    // Attempts to fetch and pop elements from the queue and stores the latest element. returns false if queue was empty from the start
    // return by reference, blocks as longs as locked by someone else, doesn't wait for queue to be filled when empty
    bool tryPop(T& elem) 
    {
        // apply lock guard for thread safe access to the queue
        const std::lock_guard<std::mutex> lock(m_mutex);
        
        // check and abort if queue is already/still empty
        if(m_queue.empty()) {
            return false;
        }

        // pop elements until queue is empty, return the last via reference
        do {
            elem = m_queue.front();
            m_queue.pop();
        } while(!m_queue.empty());

        return true;
    }

    // Pops all elements in the queue until empty
    void clear() {
        // apply lock guard for thread safe access to the queue
        const std::lock_guard<std::mutex> lock(m_mutex);
        while(!m_queue.empty()) { 
            m_queue.pop(); 
        }
    }
};

