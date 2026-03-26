#include <qtautotest/action_observer.h>

namespace qtautotest {

namespace {

ActionObserver*& observerSlot()
{
    static ActionObserver* observer = nullptr;
    return observer;
}

} // namespace

void setActionObserver(ActionObserver* observer)
{
    observerSlot() = observer;
}

ActionObserver* actionObserver()
{
    return observerSlot();
}

} // namespace qtautotest
