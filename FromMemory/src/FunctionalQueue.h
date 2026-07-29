//
// Created by johnm on 7/19/2026.
//

#ifndef FUNCTIONALQUEUE_H
#define FUNCTIONALQUEUE_H

#include "fmt/printf.h"

#include <cassert>
#include <deque>
#include <functional>
#include <utility>

/*
 *  Pretty much just what the vkguide does, but I delete the copy constructors since... I don't trust like that
 *
 *  Calls functions in inverse order of registration, thats all it do
 *
 *  I think this doesn't scale when I have a bajillion objects but that is NOT the case right now
 */
class FunctionalQueue
{
  public:
    FunctionalQueue() = default;
    ~FunctionalQueue()
    {
        if (!queue.empty())
        {
            fmt::printf("FunctionalQueue destroyed without flush.\n");
        }
    }
    FunctionalQueue(const FunctionalQueue &) = delete;
    FunctionalQueue &operator=(const FunctionalQueue &) = delete;

    // on move construct, we empty the other to ensure the functions aren't there any more
    FunctionalQueue(FunctionalQueue &&other) : queue(std::exchange(other.queue, {}))
    {
    }

    FunctionalQueue &operator=(FunctionalQueue &&other)
    {
        if (this != &other)
        {
            // ensure that this queue is empty... before moving something into it
            assert(queue.empty());
            queue = std::exchange(other.queue, {});
        }
        return *this;
    }

    // can only push back rvalue funcs and then move them into the queue
    void Register(std::function<void()> &&function)
    {
        queue.push_back(std::move(function));
    }

    // calls all em and clears the queue
    void Flush()
    {
        for (auto rit = queue.rbegin(); rit != queue.rend(); ++rit)
        {
            (*rit)();
        }
        queue.clear();
    }

  private:
    std::deque<std::function<void()>> queue;
};

#endif // FUNCTIONALQUEUE_H
