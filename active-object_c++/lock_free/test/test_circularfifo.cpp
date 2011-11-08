/* test_circularfifo.cpp
* Not any company's property but Public-Domain
* Do with source-code as you will. No requirement to keep this
* header if need to use it/change it/ or do whatever with it
*
* Note that there is No guarantee that this code will work
* and I take no responsibility for this code and any problems you
* might get if using it.
*
* 2010 @author Kjell Hedström, hedstrom@kjellkod.cc */


#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <pthread.h>

#include "circularfifo.h"
#include "active.h"
#include "activecallback.h"
#include "backgrounder.h"

#include <gtest/gtest.h>

namespace { 
  static const unsigned int c_nbrItems = 20000;
    
   /// Defines the test objects with subroutines to verify the circular fifo
  class CircularFifoTest : public ::testing::Test {
    private:
    bool createJobs();
    void printPercentage(const unsigned int nbr_, unsigned int& progress_);
    void printConsumeProgress(void);
    
    protected:
      std::auto_ptr<Backgrounder> bgWorker;
      std::vector<int> producedQ; // to verify, that all successfully sent were received
      int failedCnt;   // Cnt for how many times the test "waited" due to full queue
      std::vector<int> savedQ;    // queue for received & processed msgs
      void sendToBgWorker();

      
      virtual void SetUp() // gtest runs this for every test fixture
      {
	failedCnt = 0;
        savedQ.reserve(c_nbrItems);
        bgWorker = std::auto_ptr<Backgrounder>(new Backgrounder(savedQ));
      }

    };
    
    
    /// Create a job, add to work queue. 
    /// \return true if job was added and false if queue was full
    bool CircularFifoTest::createJobs() {
      int item = rand();
      if(false == bgWorker->saveData(item)){
        failedCnt++; // just to show how much we were punished for a fixed size queue 
        pthread_yield(); // ease off CPU w/out explicit cleeping
	return false;
      }
      else { // success
        producedQ.push_back(item);
	return true;
      }
    }
     
    
    /// Test function to send dummy data to an Active object 
    /// that processe the jobs asynchronously
    void CircularFifoTest::sendToBgWorker() {
      timeval t1, t2;
      gettimeofday(&t1, NULL);    // timer start
      srand((unsigned) time(NULL));
   
      unsigned int progress = 0;
      unsigned int cnt = 0;
      std::cout << "\tProduced [%]: ";
      
      bgWorker->timer(); // start bg timer
      while(cnt < c_nbrItems){
	if(createJobs()) // increase by one if successfully added a job
	{
	  cnt++;
	  printPercentage(cnt, progress);
	 }
	}
      // timer stop
      gettimeofday(&t2, NULL);
      std::ostringstream oss;
      oss << " (finished) in " << timeInMs(t1, t2) << " [ms]\t" << std::endl;
      std::cout << oss.str().c_str();
      
      printConsumeProgress(); // if any left
    }
  
   
    /// print out progress every 10 %
    void CircularFifoTest::printPercentage(const unsigned int nbr_, unsigned int& progress_){
      float percent = 100 * ((float)nbr_/c_nbrItems);      
      unsigned int rounded = ((int)percent/10)*10;
      if(rounded != progress_){
	std::cout << progress_ << " " << std::flush;    
	progress_ = rounded;
	return;
      }
    }
    


 void CircularFifoTest::printConsumeProgress()
 {
  std::cout << "\tConsumed [%]: ";	 
  unsigned int progress = 100;   
   while((savedQ.size() < c_nbrItems) && progress <= 100)
   {
     pthread_yield();
     unsigned int pSize = producedQ.size();
     unsigned int remaining = pSize - savedQ.size();    
     printPercentage(remaining, progress);
   }
   bgWorker->timer(); // stop bg timer & print result
 }
 
 
 /// -----------------------------------------------------------------
 ///  Test Part A
 /// -----------------------------------------------------------------  
  
  /// verify all queues are initially empty
  TEST_F(CircularFifoTest, IsEmptyInitially) {
    ASSERT_TRUE((failedCnt == 0));
    ASSERT_TRUE((savedQ.size() == 0));
    ASSERT_TRUE((producedQ.size() == 0));
  }
    
    
  /// Verify mucho work added asynchronously to worker for bg processing
  TEST_F(CircularFifoTest, BgWork) {
    timeval t1, t2;
    gettimeofday(&t1, NULL);    // start clock
    sendToBgWorker();  
    // check1: all items produced + at least someting received
    ASSERT_EQ(producedQ.size(), c_nbrItems);
    ASSERT_TRUE((savedQ.size() > 0));

    // check2: kill bg thread & finish processing. Verify transfers
    bgWorker.reset(); // kill active object & wait for it to process all jobs
    ASSERT_EQ(producedQ.size(), savedQ.size());
    gettimeofday(&t2, NULL);    // "stop" clock
    
    // check3: verify transfer integrity to processed + failed_due_to_filled_queue
    ASSERT_TRUE(std::equal(producedQ.begin(), producedQ.end(), savedQ.begin()));  
    std::ostringstream oss;
    oss << timeInMs(t1, t2) << " [ms]";
    std::cout << "\tJobs sent: " << producedQ.size();
    std::cout << ". Received: " << savedQ.size();
    std::cout << " in: " << oss.str().c_str() << std::endl;
    std::cout << "\tCircularFifo limitation manifested (full queue): "  << failedCnt << " times. " << std::endl;  
    std::cout << "\t ---Please follow the advice and use a different queue :p" << std::endl;
    std::cout << "\t --- See the updated version of active-object with C++11 at: www.kjellkod.cc/kjellkod-code " << std::endl;
    std::cout << std::flush; 
  }
  
  
   /// -----------------------------------------------------------------
 ///  Test Part B
 /// -----------------------------------------------------------------
    class CircularFifoFunc : public ::testing::Test {
    protected:
      static const int c_size = 100;
      kjellkod::CircularFifo<int, c_size> fifo;
    };

    
  TEST_F(CircularFifoFunc, Random) {
    int count = 0;
    int dummy = 0;

    while(count < c_size * 100){
       count++;

       int add = rand();
       for(int i=0; i< add; i++){
         if(!fifo.push(dummy=rand()))
           break;
        }

       int subtract = rand();
       for(int j=0; j<subtract; j++){
         if(!fifo.pop(dummy))
           break;
       }
   }
}



  TEST_F(CircularFifoFunc, Size) {
    ASSERT_TRUE(fifo.isEmpty());
    ASSERT_EQ(fifo.nbrOfItems(), 0);
        
    int item = 1;
    fifo.push(item);
    ASSERT_EQ(fifo.nbrOfItems(), 1);
    fifo.pop(item);
    ASSERT_EQ(fifo.nbrOfItems(), 0);
    ASSERT_TRUE(fifo.isEmpty());

    int added = 0;
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 4; j++) {
            if (fifo.push(item)) {
                item++;
                added++;
                ASSERT_EQ(fifo.nbrOfItems(), added);
              }
          }
        if (fifo.pop(item)) {
            added--;
            item++;
            ASSERT_TRUE(fifo.nbrOfItems() == added);
          }
      }
  }

} // namespace