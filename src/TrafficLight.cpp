#include <iostream>
#include <random>
#include "TrafficLight.h"

#include <future>
#include <queue>

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_queue.empty(); });

    T message = std::move(_queue.back());
    _queue.pop_back();
    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&message)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    //Add message to queue, then notify
    if (message)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _queue.emplace_back(std::move(message));
        _condition.notify_one();
    }
}


/* Implementation of class "TrafficLight" */


// TrafficLight::TrafficLight()
// {
//     _currentPhase = TrafficLightPhase::red;
// }

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        TrafficLightPhase currentPhase = _messageQueue->receive();
        if (currentPhase == green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called.
    //To do this, use the thread queue in the base class. 
    std::thread(&TrafficLight::cycleThroughPhases, this);
}


//helper function to toggle current phase
void TrafficLight::toggleCurrentPhase()
 {
     if (_currentPhase == green)
     {
         _currentPhase = red;
     }
     else
     {
         _currentPhase = green;
     }   
 }

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    //Generate random value between 4 and 6
    std::random_device rand;
    std::mt19937 engine(rand());
    std::uniform_int_distribution<> distr(4, 6);

    //Pick random cycle duration
    auto cycleDuration = distr(engine);

    //Initialize time tracking
    auto timeLastCycleEnd = std::chrono::system_clock::now();

    while(true)
    {
        //time elapsed between cycles = thisCycleStart - lastCycleEnd
        auto timeThisCycleStart = std::chrono::system_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(timeThisCycleStart - timeLastCycleEnd).count();

        //Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        //Wait for end of cycle duration
        if(timeElapsed >= cycleDuration)
        {
            //toggle current phase
            toggleCurrentPhase();

            //send update with move semantics, wait to complete
            TrafficLightPhase message= _currentPhase;
            auto future = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _messageQueue, std::move(message));
            future.wait();

            //Reset next cycle duration
            cycleDuration = distr(engine);

            //Reset time tracking for next cycle
            timeLastCycleEnd = std::chrono::system_clock::now();
        }
    }
}


