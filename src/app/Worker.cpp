#include "Worker.hpp"
#include "App.hpp"
#include "../common/println.hpp"
#include<thread>

Worker::Worker ():
running(false)   
{}

void Worker::start () {
std::thread t([this](){ run(); });
t.detach();
}

void Worker::submit (const Task& task) {
taskQueue.Post(task);
}

void Worker::stop () {
running=false;
submit([](){});
}

void Worker::run () {
try {
Task task;
running=true;
println("Starting worker thread");
while(running) {
taskQueue.Receive(task);
task();
}
println("Stopping worker thread");
} catch (std::exception& e) {
println("Exception! {}: {}", typeid(e).name(), e.what());
}
}

