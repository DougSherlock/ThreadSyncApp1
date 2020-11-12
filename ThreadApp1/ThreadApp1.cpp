// Purpose: Application which uses a condition variable to synchronize threads in C++.
// Source:  https://www.youtube.com/watch?v=kpAY-BGwwZk

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>


using namespace std;

// mutex and condition variable will be used to synchronize threads
mutex g_mutex;
condition_variable g_condition;

bool inputReady = false
, resultReady = false
, outputDone = false
, stopCalcThread = false
, stopOutputThread = false;

void CalculateSquare(const int& input, int& result)
{
    do {
        unique_lock<mutex> lk(g_mutex);
        g_condition.wait(lk, [] { return (inputReady || stopCalcThread); }); // wait for input to be ready or thread to be terminated
        if (inputReady) {
            result = input * input;
            inputReady = false;
            resultReady = true;
            lk.unlock();
            while (resultReady) {
                g_condition.notify_one();
            }
        }

        if (stopCalcThread) {
            stopCalcThread = false;
            stopOutputThread = true;
            lk.unlock();
            while (stopOutputThread) {
                g_condition.notify_one();
            }
            break;
        }
    } while (true);
}

void OutputSquare(const int& result) 
{
    do {
        unique_lock<mutex> lk(g_mutex);
        g_condition.wait(lk, [] { return (resultReady || stopOutputThread); }); // wait for result to be ready or thread to be terminated
        if (resultReady) {
            cout << "z = " << result << endl;
            resultReady = false;
            outputDone = true;
            lk.unlock();
            while (outputDone) {
                g_condition.notify_one();
            }
        }

        if (stopOutputThread) {
            stopOutputThread = false;
            lk.unlock();
            break;
        }
    } while (true);
}

int main()
{
    int input, result;
    char cmd('n');
    thread calcThread(CalculateSquare, cref(input), ref(result)); // Note: cref and ref are used to bind parameters to a function
    thread outputThread(OutputSquare, cref(result));
    do {
        unique_lock<mutex> lk(g_mutex); // lock "x"
        cout << "Enter x: ";
        cin >> input; // set "x"
        inputReady = true; // x has been set but not processed
        lk.unlock(); // unlock "x"
        while (inputReady) {
            g_condition.notify_one(); // If any threads are waiting on *this, calling notify_one unblocks one of the waiting threads
        }

        lk.lock();
        g_condition.wait(lk, [] { return outputDone; }); // wait for the result to be displayed
        outputDone = false;
        cout << "Continue? (y/n): ";
        cin >> cmd;
    } while (cmd == 'y');

    stopCalcThread = true;
    while (stopCalcThread) {
        g_condition.notify_one();
    }

    calcThread.join();
    outputThread.join();
    return 0;
}
