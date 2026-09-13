#include<iostream>
#include<thread>
#include<mutex>
int counter = 0;
std::mutex mtx;
void worker(){
    for(int i = 0; i< 100000; i++){
       mtx.lock();
        counter++;
        return;
       mtx.unlock();
    }
}

int main(){
   std::thread t1(worker);
   std::thread t2(worker);

   t1.join();
   t2.join();

   std::cout << counter << std::endl;
}