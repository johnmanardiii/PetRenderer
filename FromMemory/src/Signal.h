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

    Signal(Signal && other) noexcept = default;
    Signal &operator=(Signal && other) noexcept = default;

    int connect(std::function<void(Args...)> const& slot)
    {
        m_map.insert(std::make_pair(++m_iCurrentID, slot));
        return m_iCurrentID;
    }

    void disconnect(int id)
    {
        m_map.erase(id);
    }

    void disconnect_all()
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
