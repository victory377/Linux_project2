Linux OS
===
## Project 需求
* [x] `go_to_sleep_now()` 可以把呼叫這個Function的程序放到Wait Queue. 
* [x] Wait Queue是由你自己定義的（命名：my_wait_queue）。
* [x] 使用上述的系統呼叫去寫一個程式（ test_wait_queue.c）如下。
* [x] 寫一個程式（wake_up_process.c）如下。這個程式接收一個參數。
* [x] 這個參數是一個Process的Global PID，而這個Process是呼叫`go_to_sleep_now()`後被放在my_wait_queue的Process。
* [x] 在wake_up_process.c裡面你要使用另外一個新的系統呼叫`wake_up_my_process()`去特別喚醒這個Process。

## 評分標準
* [x] [50分] 成功完成Project要求的功能
* [x] [25分] 詳細的敘述wait_queue的機制
* [x] [20分] 解釋你增加的程式碼(System Call)
* [x] 解釋任何有關於你Project的問題例如：
`the sleep time of your process` 與 在你呼叫`go_to_sleep_now()`  和 `wake_up_my_process()` 時的 `the length of the interval` 之間的關係？



## 作業環境

__CPU__ 			Intel(R) Core(TM) i7 CPU  870 @ 2.93GHz, 1200 MHz
__RAM__ 			6G
__OS__ 				Ubuntu 13.04(x86)
__Kernel Version__ 	3.9.8

#### 執行步驟
1. 執行`test_wait_queue.c`
2. 執行`wake_up_process.c`並依照指示輸入`test_wait_queue.c`之PID

### 實作結果

###### test_wait_queue
~~~
root@linux:~# ./test_wait_queue 
Pid:3602
It takes the process 40515104 uses to complete the double loop, if the process has slept for a while.
It takes the process 20078627 uses to complete the double loop, if the process does not sleep
It takes about 20436477 to sleep.
~~~
###### wake_up_process
~~~
root@linux:~# ./wake_up_process
Input the global PID of the process you want to wake up.
Pid: 3602
Format: program PID
Process 3602 has been waked up.
~~~


## 問題與討論
### Kernel Space

1.Runqueue存放者執行中的process，哪一些函式改變process's state及將process加入waitqueue中，並把原先存在Runqueue中已經在waitqueue的刪除?


>我們是使用`prepare_to_wait()`實作，將要sleep的process加入我們自定義的waitqueue中，但runqueue中process還是存在者，因為process狀態改變為TASK_INTERRUPTIBLE或TASK_UNINTERRUPTIBLE，所以透過呼叫`schedule()`就會將狀態非TASK_RUNNING的process從runqueue中移除。


2.從Waitqueue中被喚醒的process如何在被加入Runqueue中?

>我們用`DEFINE_WAIT()`，去初始化我們waitqueue的節點，`DEFINE_WAIT()`喚醒的function是使用`autoremove_wake_function()`，因為`autoremove_wake_fuction()`裡面有呼叫 `defualt_wake_function()`，而`defualt_wake_function()`裡面有呼叫 `try_to_wake_up()` ，`try_to_wake_up()`負責將喚醒的process設定其狀態為TASK_RUNNING，並將proces s移出waitqueue，加入到runqueue中讓排班程式選擇。

3.一開始將`go_to_sleep_now()`與`wake_my_process()`兩個systemcall分開寫在不同的.c檔，所以遇到了共用變數的問題?

>我們這組討論很久，後來直接把2個systemcall寫在相同的.c檔內，這樣就不用怕waitqueue的head找不到，後來編譯過後測試，可以正常實作出waitqueue的功能。

### 架構分析
當我們宣告一個My Wait Queue時，系統會幫我們創建一個Wait Queue Head，其中包含了一個SipnLock和一組Link List的結構。
~~~ c
#define DECLARE_WAIT_QUEUE_HEAD(name)
	wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {				\
	.lock        = __SPIN_LOCK_UNLOCKED(name.lock),			\
	.task_list   = { &(name).task_list, &(name).task_list }	\
}
~~~
當我們定義了一個 `wait_queue_t` 時，Kernel會對他進行初始化，並且定義其中的func為 `default_wake_function()` 
~~~ c
#define DEFINE_WAIT(name) 									\
	DEFINE_WAIT_FUNC(name, autoremove_wake_function)
	
#define DEFINE_WAIT_FUNC(name, function)         			\
	wait_queue_t name = {                            		\
		.private        = current,                       	\
		.func           = function,                      	\
		.task_list      = LIST_HEAD_INIT((name).task_list),	\
}

static inline void init_waitqueue_entry(wait_queue_t *q, struct task_struct *p) {
	q->flags = 0;
	q->private = p;
	q->func = default_wake_function;
}

int default_wake_function(wait_queue_t *curr, unsigned mode, int wake_flags,void *key) {
        return try_to_wake_up(curr->private, mode, wake_flags);
}
~~~
接下來我們呼叫`prepare_to_wait_exclusive()` 讓我們的程序加入我們自定義的my wait queue中，再呼叫 `schedule()` 把系統控制權交還給Kernel。
~~~ c
prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state) {
         unsigned long flags;
         
         wait->flags |= WQ_FLAG_EXCLUSIVE;
         spin_lock_irqsave(&q->lock, flags);
         if (list_empty(&wait->task_list))
                 __add_wait_queue_tail(q, wait);
         set_current_state(state);
         spin_unlock_irqrestore(&q->lock, flags);
}
~~~
最後在喚醒的部分，使用 `wait_queue_t` 初始化時定義的func-> `try_to_wake_up()` 會把睡眠中的程式重新啟動，把它放回Run Queue中，等待CPU執行剩餘的程式碼。
### 結論

### 心得

__簡宇謙__

> 這次的project2感覺相對於上次project1較為簡單，我想是因為經過project1的經驗累積的關係。回想之後也發現，當時實作project1時課堂進度是還沒教到task struct的，因此對於很多kernel的資料結構都不甚理解，也就只能盲目的四處亂搜尋，因此時間花費非常多。而本次的project2是在上課時理解過wait queue的詳細內容才實作的，在有整體架構的概念下就很容易知道該如何去解決project2的問題，也因此除了複習課堂的觀念所花的時間外，真正撰寫code所花的時間不超過半天，並且由於先前的經驗打底，在沒有什麼bug的情況下馬上就完成並達到題目的要求，是一件值得慶幸的事。由於時間是學期末，因此事實上很多事情都需要在短時間內完成，這裡必須感謝組員們的諒解，能夠讓我在先完成其他課業後才加入project2的動工，並如期完成。

__黃育凱__

> 這次作業是實做一個自定義的waitqueue，以前大學在學作業系統時，常常在Process Synchronization那個單元聽到，對於waitqueue只是知道理論的知識，當執行中process正在等待event發生，為了提高Cpu的throughput，就會將Process放入waitqueue中，一旦event觸發了，就將process移到runqueue中，經由排班程式選出最高優先權的process，就可以使用Cpu，因為上課時，有些聽不太懂，後來把投影片waitqueue及錄影重新看過一次，不懂的地方與隊友們互相討論，一開始不是很懂`sleep_on()`為何不太好，因為它可能正要把process放到wait_queue時，就被interrupt而waitqueue中並沒有該proce ss，待interrupt結束後，process就被放到waitqueue中，這樣就造成process就一直睡在waitqueue中，process就無法正常被喚醒，我們雖然中間遇到兩個systemcall共享變數的問題，後來經由討論直接將兩個systemcall寫在相同的.c檔內，這個問題就被解決了，後續還有遇到尋找哪一個函式負責從runqueue移除非running state的process，與哪一個函式把被wake後的process重新加回runqueue中，透過與隊友們討論及trace code，抽絲剝繭的方式找到最後的解法，感謝一起努力的隊友，才能將這次project完成。
	
__蔡明勳__

> 在這次的Project中，我們一開始不是很清楚Wait Queue在Linux中的操作方式，其實對於整個Kernel的排程都不是很清楚。後來我們三個組員回去重聽了老師上課的錄影帶，再經過組內互相討論之後，我們才比較知道Kernel Code在座什麼事情。這次的作業有上次的Project1的經驗，在開發改寫上快很多。這次我還是要感謝我的組員這麼凱瑞，跟他們一起討論讓我受益良多。



***

## Kernel Source Code

~~~ c
#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/wait.h>			//wait_queue中許多函式使用
#include <linux/sched.h>		//排班程式使用

static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue); // 宣告wait_queue_head

asmlinkage void sys_go_to_sleep_now ( void )  //sleep_sys_call
{
	DEFINE_WAIT(wait);
	prepare_to_wait_exclusive(&my_wait_queue, &wait, TASK_UNINTERRUPTIBLE); 
	schedule();         //讓出cpu                    
	finish_wait(&my_wait_queue, &wait);
}

asmlinkage int sys_wake_up_my_process(int pid) //wake_up system call 成功回傳0 失敗回傳-1
{
   	int value = -1;
	wait_queue_t *curr, *next; 
   	struct task_struct *target_task;
	list_for_each_entry_safe(curr, next, &my_wait_queue.task_list, task_list) {
		target_task = curr->private;
		if( pid == target_task->pid ) {
			curr->func(curr, TASK_UNINTERRUPTIBLE, 1, NULL);
			value = 0;
			break;  
		}
	}
    return value;   
}
~~~

## User Space Source Code (1)
~~~ c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

main(int argc, char **argv) {
	int pid;
	printf("Input the global PID of the process you want to wake up.\n");
	printf("Pid: ");
	scanf(" %d",&pid);
	printf("Format: program PID\n");
	if(syscall(353,pid)<0) { //new system call two
		printf("Cannot wake up process %d\n", pid);
	}
	else
		printf("Process %d has been waked up.\n",pid);

}
~~~

## User Space Source Code (2)

~~~ c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

main(int argc, char **argv){ 
	unsigned i, j, flag=0;
	struct timeval	start, end;
	gettimeofday(&start, NULL);
	for(i=0;i<100000;i++)
		for(j=0;j<100000;j++)
			if(flag==0) {
				syscall(352);
				flag=1;
			}
	gettimeofday(&end, NULL);
	printf("It takes the process %ld uses to complete the double loop, 
	if the process has slept for a while.\n", ((end.tv_sec * 1000000 + \
	end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));

	gettimeofday(&start, NULL);
	for(i=0;i<100000;i++)
		for(j=0;j<100000;j++)
			;
	gettimeofday(&end, NULL);
	printf("It takes the process %ld uses to complete the double loop, 
	if the process does not sleep\n", ((end.tv_sec * 1000000 + end.tv_usec) - \
	(start.tv_sec * 1000000 + start.tv_usec)));
}
~~~

### Reference
1.	https://hwchen18546.wordpress.com/2014/04/10/linux-trace-task_struct/ 
2.	http://www.linuxjournal.com/article/8144
3.	http://blog.csdn.net/hongchangfirst/article/details/7075026   
4.	http://blog.csdn.net/hongchangfirst/article/details/7076225   
5.	http://www.jollen.org/blog/2008/07/process_state_wait_queue.html
6.	http://huenlil.pixnet.net/blog/post/25190567-%5B
7.	http://lxr.free-electrons.com/