/* mtk_c.c */
#include <stdio.h>
#include "mtk_c.h"

/* 大域変数の実体定義 */
TASK_ID_TYPE curr_task;
TASK_ID_TYPE new_task;
TASK_ID_TYPE next_task;
TASK_ID_TYPE ready;

SEMAPHORE_TYPE semaphore[NUMSEMAPHORE];
TCB_TYPE task_tab[NUMTASK+1];
STACK_TYPE stacks[NUMTASK];

/* ======================================
   init_kernel: カーネルの初期化
   ====================================== */
void init_kernel()
{
    int i;
    for (i = 1; i <= NUMTASK; i++) {
        task_tab[i].status = UNDEFINED;
        task_tab[i].next = NULLTASKID;
    }
    ready = NULLTASKID;
    
    /* TRAP #1 (Vector 33 = 0x84) に pv_handler を登録 */
    *(void (**)())0x84 = pv_handler;

    for (i = 0; i < NUMSEMAPHORE; i++) {
        semaphore[i].count = 0;
        semaphore[i].task_list = NULLTASKID;
    }
    
    //semaphore[0].count = 1;
}

/* ======================================
   init_stack: スタックフレームの初期化 
   ====================================== */
void* init_stack(TASK_ID_TYPE id)
{
    /* スタックは配列の末尾(高位アドレス)から使用する */
    unsigned long *ssp = (unsigned long *)&stacks[id - 1].sstack[STKSIZE];


    /* 1. Initial PC (4byte) - 一番底(High)に配置 */
    /* first_task の最後の RTE で取り出される */
    *(--ssp) = (unsigned long)task_tab[id].task_addr;

    /* 2. Initial SR (2byte) */
    unsigned short *ssp_w = (unsigned short *)ssp;
    *(--ssp_w) = 0x0000; /* User Mode, Interrupt Enabled */

    /* 3. レジスタ退避領域 (15本 * 4byte = 60byte分空ける) */
    /* ssp_w は short型ポインタなので、char*にキャストしてバイト単位で計算 */
    ssp = (unsigned long *)((char *)ssp_w - 60);

    /* 4. Initial USP (4byte) - 一番上(Low)に配置 */
    /* first_task で最初に取り出される */
    *(--ssp) = (unsigned long)&stacks[id - 1].ustack[STKSIZE];

    /* 戻り値は現在のスタックトップ (USPが入っている場所) */
    return (void *)ssp;
}

/* ======================================
   set_task: タスクの登録
   ====================================== */
void set_task(void (*usertask_ptr)())
{
    int id;
    new_task = NULLTASKID;
    for (id = 1; id <= NUMTASK; id++) {
        if (task_tab[id].status == UNDEFINED) {
            new_task = id;
            break;
        }
    }
    if (new_task == NULLTASKID) return;

    task_tab[new_task].task_addr = usertask_ptr;
    task_tab[new_task].status = READY;
    task_tab[new_task].stack_ptr = init_stack(new_task);

    addq(&ready, new_task);
}

/* ======================================
   begin_sch: スケジューリング開始
   ====================================== */
void begin_sch()
{
    curr_task = removeq(&ready);
    /* init_timer(); */ /* テストD1段階ではコメントアウト*/
    first_task();
}

/* ======================================
   sched: スケジューラ
   ====================================== */
void sched(void)
{
    next_task = removeq(&ready);
    /* 実行すべきタスクがなくなったら無限ループ (アイドル状態) */
    if(next_task == NULLTASKID) while(1);
}

/* ======================================
   addq: キューへの追加
   ====================================== */
void addq(TASK_ID_TYPE* que_ptr, TASK_ID_TYPE new_task_id)
{
    if(*que_ptr == NULLTASKID){
        *que_ptr = new_task_id;
        task_tab[new_task_id].next = NULLTASKID; /* 安全のため終端処理 */
    }
    else{
        TCB_TYPE* task_ptr = &task_tab[*que_ptr];
        while(1){
            if((*task_ptr).next == NULLTASKID){
                (*task_ptr).next = new_task_id;
                task_tab[new_task_id].next = NULLTASKID; /* 終端処理 */
                return; 
            }
            else{
                task_ptr = &task_tab[(*task_ptr).next];
            }
        }
    }
}

/* ======================================
   removeq: キューからの取り出し
   ====================================== */
TASK_ID_TYPE removeq(TASK_ID_TYPE* que_ptr){
    TASK_ID_TYPE top_id = *que_ptr;
    if(top_id != NULLTASKID){
        TCB_TYPE* task_ptr = &task_tab[top_id];
        *que_ptr = (*task_ptr).next;
        (*task_ptr).next = NULLTASKID;
    }
    return top_id;
}

/* ======================================
   p_body, v_body, sleep, wakeup
   ====================================== */
void p_body(SEMAPHORE_ID_TYPE sem_id)
{
    SEMAPHORE_TYPE *sem = &semaphore[sem_id];
    sem->count--;
    
    printf("[P] sem_id=%d, count=%d\r\n", sem_id, sem->count);
    
    if (sem->count < 0) {
        sleep(sem_id);
    }
}

void v_body(SEMAPHORE_ID_TYPE sem_id)
{
    SEMAPHORE_TYPE *sem = &semaphore[sem_id];
    sem->count++;
    
    printf("[V] sem_id=%d, count=%d\r\n", sem_id, sem->count);
    
    if (sem->count <= 0) {
        wakeup(sem_id);
    }
}

void waitp_body(SEMAPHORE_ID_TYPE sem_id){
	SEMAPHORE_TYPE *sp;
	sp = &semaphore[sem_id];
	if(sp->count != -(sp->nst-1)) p_body(sem_id);
	else{
		for(int k=0; k<sp->nst-1; k++){
			v_body(sem_id);
			addq(&ready, curr_task);
			sched();
			swtch();
		}
	}
}

void sleep(SEMAPHORE_ID_TYPE sem_id){
    addq(&semaphore[sem_id].task_list, curr_task);
    printf("[sleep] task=%d on sem=%d, count=%d\r\n", curr_task, sem_id, semaphore[sem_id].count);
    sched();
    swtch();
}

void wakeup(SEMAPHORE_ID_TYPE sem_id){
    TASK_ID_TYPE id = removeq(&semaphore[sem_id].task_list);
    if (id != NULLTASKID) {
        addq(&ready, id);
        printf("[wakeup] task=%d from sem=%d, count=%d, ready_head=%d\r\n",
               id, sem_id, semaphore[sem_id].count, ready);
    }
}

