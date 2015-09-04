#include <iostream>
#include <pthread.h>
#include <mutex>

using namespace std;

mutex mtx;

void *print_block (char* c) {
  char tmp = *c;
  mtx.lock();
  for (int i=0; i<50; ++i) { std::cout << c; }
  cout << '\n';
  mtx.unlock();
}

int main ()
{
  //pthread_t th1 (print_block,50,'*');
  //pthread_t th2 (print_block,50,'$');
  char c1 = '*';
  char c2 = '$';

  pthread_t thread1;
  pthread_create(&(thread1), NULL, print_block, &c1);

  pthread_t thread2;
  pthread_create(&(thread2), NULL, print_block, &c2);

  pthread_join(thread1,NULL);
  pthread_join(thread2,NULL);
  
  //th1.join();
  //th2.join();

  return 0;
}