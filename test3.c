/* test3.c: fscanf / scanf を使った相互通信 (チャット) */
#include <stdio.h>
#include "mtk_c.h"

extern void init_uart2(void);

FILE *fp_com0;
FILE *fp_com1;

/* ----------------------------------------------------
   Task 0: COM0入力を監視 -> COM1へ送信
   ---------------------------------------------------- */
void task_com0()
{
    char buf[64];
    
    printf("\r\nTask0(COM0) Ready. Type message for COM1.\r\n");

    while(1) {
        printf("COM0> "); 
        
        /* ★修正: scanfを使用 (要件準拠) */
        /* 空白または改行が来るまで読み込みます */
        if (scanf("%s", buf) > 0) {
            
            /* 1. 相手(COM1)に送る */
            if (fp_com1 != NULL) {
                fprintf(fp_com1, "\r\n\n[From COM0]: %s", buf);
                fprintf(fp_com1, "\r\nCOM1> "); 
                /* バッファリング無効化していても念のためflush */
                fflush(fp_com1);
            }
        }
    }
}

/* ----------------------------------------------------
   Task 1: COM1入力を監視 -> COM0へ送信
   ---------------------------------------------------- */
void task_com1()
{
    char buf[64];

    if (fp_com1 != NULL) {
        fprintf(fp_com1, "\r\nTask1(COM1) Ready. Type message for COM0.\r\n");
    }

    while(1) {
        if (fp_com1 != NULL) {
            fprintf(fp_com1, "COM1> ");
            fflush(fp_com1);

            /* ★修正: fscanfを使用 (要件準拠) */
            /* ファイルポインタ fp_com1 から文字列を読み込みます */
            if (fscanf(fp_com1, "%s", buf) > 0) {
                
                /* 1. 相手(COM0)に送る */
                if (fp_com0 != NULL) {
                    fprintf(fp_com0, "\r\n\n[From COM1]: %s", buf);
                    fprintf(fp_com0, "\r\nCOM0> "); 
                    fflush(fp_com0);
                }
            }
        }
    }
}

/* ----------------------------------------------------
   Main
   ---------------------------------------------------- */
void main()
{
    init_kernel();
    init_uart2(); /* ハードウェア初期化 */

    fp_com0 = fdopen(3, "r+");
    fp_com1 = fdopen(4, "r+");

    if (fp_com0 == NULL || fp_com1 == NULL) while(1);

    /* fscanfを正しく動かすためにバッファリング無効化は必須 */
    setvbuf(fp_com0, NULL, _IONBF, 0);
    setvbuf(fp_com1, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Multi-Port Chat (fscanf version) Start.\r\n");

    set_task(task_com0);
    set_task(task_com1);

    begin_sch();
}
