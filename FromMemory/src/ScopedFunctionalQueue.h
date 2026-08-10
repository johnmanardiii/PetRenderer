//
// Created by johnm on 7/26/2026.
//

#ifndef SCOPEDFUNCTIONALQUEUE_H
#define SCOPEDFUNCTIONALQUEUE_H

#include "FunctionalQueue.h"

/*
 *  Like a functional queue, but on destruction, it calls all of the functions in the queue
 *
 *  Supports moving, where then the moved-to version "owns" the functions to be called on dest
 */
class ScopedFunctionalQueue
{
public:
    ScopedFunctionalQueue() = default;
    ~ScopedFunctionalQueue()
    {
        queue.Flush();
    }
    ScopedFunctionalQueue(const ScopedFunctionalQueue& other) = delete;
    ScopedFunctionalQueue& operator=(const ScopedFunctionalQueue& other) = delete;
    ScopedFunctionalQueue(ScopedFunctionalQueue&& other) = default;
    ScopedFunctionalQueue& operator=(ScopedFunctionalQueue&& other) = default;

    void Register(std::function<void()>&& function)
    {
        queue.Register(std::move(function));
    }
    void Flush()
    {
        queue.Flush();
    }
private:
    FunctionalQueue queue;
};

#endif //SCOPEDFUNCTIONALQUEUE_H
