
#include "KeyEventNotifier.h"

#include <vector>
#include <mutex>
#include <algorithm>

KeyEventNotifier& KeyEventNotifier::GetSingleton() {
    static KeyEventNotifier KeyEventNotifier;
    return KeyEventNotifier;
};

void KeyEventNotifier::notify(const KeyEvent& event) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    for(auto& subscriber : subscribers) {
        subscriber->OnEvent(event);
    }
};

void KeyEventNotifier::subscribe(KeyEventObserver& observer) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    subscribers.push_back(&observer);
};

void KeyEventNotifier::unsubscribe(KeyEventObserver& observer) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    auto observerIter = std::find(subscribers.begin(), subscribers.end(), &observer);
    if(observerIter == subscribers.end())
        return;
    subscribers.erase(observerIter);
};