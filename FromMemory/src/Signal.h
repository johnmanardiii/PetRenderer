//
// Created by johnm on 7/15/2026.
//

#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <map>

/*
 * Pretty much based off of this for the most part:
 * https://schneegans.github.io/tutorials/2015/09/20/signal-slot
 *
 * Though I read through it and the logical constness of the class listed doesn't make sense to me so I removed it
 */
template <typename... Args>
class Signal {
public:
    Signal() = default;
    ~Signal() = default;

    // For now, going to delte the copy constructor since that makes sense to me
    // the guide has it initialize everything as empty though
    Signal(const Signal &) = delete;
    Signal &operator=(const Signal &) = delete;

    Signal(Signal && other) noexcept
    {
        m_map(std::move(other.m_map));
        // #JOHN - I need to rewatch that cppcon talk about move constructors and how to write them for the example
        // can't remember if I'm supposed to reset the primitive type values
        // and i vaguely remember a convenience function for this...
        m_iCurrentID = std::move(other.m_iCurrentID);
    }
    Signal &operator=(Signal && other) noexcept
    {
        if (this != &other)
        {
            // #JOHN - oh yeah you need to avoid the assignment to self... really need to watch that cpp con talk again
            m_map(std::move(other.m_map));
            m_iCurrentID = std::move(other.m_iCurrentID);
        }

        return *this;
    }

    int connect(std::function<void(Args...)> const& slot)
    {
        m_map.insert(std::make_pair(++m_iCurrentID, slot));
        return m_iCurrentID;
    }

    int disconnect(int id)
    {
        m_map.erase(id);
    }

    int disconnect_all()
    {
        m_map.clear();
    }

    void emit(Args... p)
    {
        for (const auto& [id, func] : m_map)
        {
            std::invoke(func, p...);
        }
    }

private:
    std::map<int, std::function<void(Args...)>> m_map;
    int m_iCurrentID = 0;
};



#endif //SIGNAL_H
