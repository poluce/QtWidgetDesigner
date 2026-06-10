#include <qtautotest/action_observer.h>

#include <atomic>

namespace qtautotest {

namespace {

std::atomic<ActionObserver*>& observerSlot()
{
    static std::atomic<ActionObserver*> observer = nullptr;
    return observer;
}

} // namespace

void setActionObserver(ActionObserver* observer)
{
    observerSlot().store(observer, std::memory_order_release);
}

ActionObserver* actionObserver()
{
    return observerSlot().load(std::memory_order_acquire);
}

} // namespace qtautotest
