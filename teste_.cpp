#include <iostream>
#include <string>
#include <syncstream>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>


class barrier{
    private:
        std::mutex m;
        std::condition_variable cv;
        int desired_count;
        int current_count;
    public:
        barrier(int c){
            desired_count = c;
            current_count = 0;
        };
        void wait_(){
            m.lock();
            current_count++;
            while (current_count < desired_count){
            cv.wait(m);
        }
            cv.notify_all();
            m.unlock();
        };
};
int main(){

return 0;    
}