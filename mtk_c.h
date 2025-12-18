/* mtk_c.h */
#ifndef _mtk_c_h_
#define _mtk_c_h_

#define NULLTASKID 0
#define NUMTASK 5
#define STKSIZE 1024
#define NUMSEMAPHORE 3

#define UNDEFINED 0
#define READY     1
#define RUNNING   2
#define WAITING   3

typedef int TASK_ID_TYPE;
typedef int SEMAPHORE_ID_TYPE;

typedef struct
{
  int count;
  int nst;
  TASK_ID_TYPE task_list;
} SEMAPHORE_TYPE;

typedef struct
{
  void (*task_addr)();
  void *stack_ptr;
  int priority;
  int status;
  TASK_ID_TYPE next;
} TCB_TYPE;

typedef struct
{
  char ustack[STKSIZE];
  char sstack[STKSIZE];
} STACK_TYPE;

/* 大域変数 (extern宣言) */
extern TASK_ID_TYPE curr_task;
extern TASK_ID_TYPE ready;
extern SEMAPHORE_TYPE semaphore[NUMSEMAPHORE];
extern TCB_TYPE task_tab[NUMTASK+1];
extern STACK_TYPE stacks[NUMTASK];

/* 関数プロトタイプ */
extern void outbyte(unsigned char c, int port); 
extern char inbyte(int port);

/* マルチタスク関連 */
void init_kernel();
void set_task(void (* usertask_ptr)());
void* init_stack(TASK_ID_TYPE id); 
void begin_sch();
void addq(TASK_ID_TYPE* que_ptr, TASK_ID_TYPE id);
TASK_ID_TYPE removeq(TASK_ID_TYPE* que_ptr);
void sched();

/* アセンブラ関数 */
extern void first_task(); 
extern void swtch();      
extern void init_timer(); 
extern void pv_handler(); 
extern int P(int sem_id);
extern int V(int sem_id);
extern void skipmt();
extern void RESET_TIMER();
extern void SET_TIMER();
extern void waitP(SEMAPHORE_ID_TYPE sem_id);

/* セマフォ関連 */
void p_body(SEMAPHORE_ID_TYPE sem_id);
void v_body(SEMAPHORE_ID_TYPE sem_id);
void sleep(SEMAPHORE_ID_TYPE sem_id);
void wakeup(SEMAPHORE_ID_TYPE sem_id);

#endif
