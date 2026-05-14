#ifndef _____WORKER_THREAD_1
#define _____WORKER_THREAD_1
#include <wx/msgqueue.h>
#include<functional>
#include<memory>

struct Worker {
typedef std::function<void()> Task;
wxMessageQueue<Task> taskQueue;
bool running;

Worker ();
void submit (const Task&);
void start ();
void stop ();
void run ();
};


#endif
