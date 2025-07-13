
#pragma once

#include <vector>
#include <mutex>
#include <algorithm>

#include "KeyEventObserver.h"

class KeyEventNotifier {
public:
    KeyEventNotifier(KeyEventNotifier& ) = delete;
    KeyEventNotifier operator=(KeyEventNotifier& ) = delete;

    static KeyEventNotifier& GetSingleton();

    friend class KeyEventObserver;
    void notify(const KeyEvent& event);

private:
    std::mutex subscribersMutex;
    std::vector<KeyEventObserver*> subscribers;

    KeyEventNotifier() = default;

    void subscribe(KeyEventObserver& observer);

    void unsubscribe(KeyEventObserver& observer);
};