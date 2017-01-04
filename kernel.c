//
//  main.c
//  kernel
//
//  Created by victory377 on 2017/1/4.
//  Copyright © 2017年 victory377. All rights reserved.
//

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
