#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this]
               { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    //std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    _condition.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto message = _msgQueue->receive();
        if (message == green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{

    std::random_device random;
    std::mt19937 gendata(random());
    std::uniform_int_distribution<int> dist(4000, 6000);
    int cycleDuration = dist(gendata);

    auto lastSwitchedTime = std::chrono::system_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto timeSinceLastTime = std::chrono::duration_cast<std::chrono::milliseconds>
                                (std::chrono::system_clock::now() - lastSwitchedTime);
        int durationSinceSwitched = timeSinceLastTime.count();
        
        if(durationSinceSwitched >= cycleDuration){
            if (_currentPhase == red)
                _currentPhase = green;
            else if (_currentPhase == green)
                _currentPhase = red;


            auto msg = _currentPhase;
            auto sentCondition = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _msgQueue, std::move(msg));
            sentCondition.wait();

            lastSwitchedTime = std::chrono::system_clock::now();
            cycleDuration = dist(gendata);
        }
        
    }
    
}
