#pragma once

#include <functional>

class KeyEventNotifier;

struct KeyEvent {
    const int key;
    const int action;

    KeyEvent(const int key, const int action) : key(key), action(action)
    {};
};

class KeyEventObserver {
public:
    KeyEventObserver(std::function<void(const KeyEvent&)> OnEvent);
    KeyEventObserver(const KeyEventObserver& other);
    ~KeyEventObserver();

private:

    friend class KeyEventNotifier; // only the notifier is able to call onEvent
    std::function<void(const KeyEvent&)> OnEvent; 
};