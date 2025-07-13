
#include "KeyEventObserver.h"
#include "KeyEventNotifier.h"


KeyEventObserver::KeyEventObserver(std::function<void(const KeyEvent&)> OnEvent) : OnEvent(OnEvent) {
    KeyEventNotifier::GetSingleton().subscribe(*this);
}

KeyEventObserver::KeyEventObserver(const KeyEventObserver& other) : OnEvent(other.OnEvent) {
    KeyEventNotifier::GetSingleton().subscribe(*this);
}

KeyEventObserver::~KeyEventObserver() {
    KeyEventNotifier::GetSingleton().unsubscribe(*this);
}