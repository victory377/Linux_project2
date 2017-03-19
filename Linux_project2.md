Linux_project2
===
## Project 需求
* [x] `go_to_sleep_now()` 可以把呼叫這個Function的程序放到Wait Queue. 
* [x] Wait Queue是由你自己定義的（命名：my_wait_queue）。
* [x] 使用上述的系統呼叫去寫一個程式（ test_wait_queue.c）如下。
* [x] 寫一個程式（wake_up_process.c）如下。這個程式接收一個參數。
* [x] 這個參數是一個Process的Global PID，而這個Process是呼叫`go_to_sleep_now()`後被放在my_wait_queue的Process。
* [x] 在wake_up_process.c裡面你要使用另外一個新的系統呼叫`wake_up_my_process()`去特別喚醒這個Process。

## 問題與討論
### Kernel Space

1.Runqueue存放者執行中的process，哪一些函式改變process's state及將process加入waitqueue中，並把原先存在Runqueue中已經在waitqueue的刪除?


>我們是使用`prepare_to_wait()`實作，將要sleep的process加入我們自定義的waitqueue中，但runqueue中process還是存在者，因為process狀態改變為TASK_INTERRUPTIBLE或TASK_UNINTERRUPTIBLE，所以透過呼叫`schedule()`就會將狀態非TASK_RUNNING的process從runqueue中移除。


2.從Waitqueue中被喚醒的process如何在被加入Runqueue中?

>我們用`DEFINE_WAIT()`，去初始化我們waitqueue的節點，`DEFINE_WAIT()`喚醒的function是使用`autoremove_wake_function()`，因為`autoremove_wake_fuction()`裡面有呼叫 `defualt_wake_function()`，而`defualt_wake_function()`裡面有呼叫 `try_to_wake_up()` ，`try_to_wake_up()`負責將喚醒的process設定其狀態為TASK_RUNNING，並將proces s移出waitqueue，加入到runqueue中讓排班程式選擇。

3.一開始將`go_to_sleep_now()`與`wake_my_process()`兩個systemcall分開寫在不同的.c檔，所以遇到了共用變數的問題?

>我們這組討論很久，後來直接把2個systemcall寫在相同的.c檔內，這樣就不用怕waitqueue的head找不到，後來編譯過後測試，可以正常實作出waitqueue的功能。