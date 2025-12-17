#include <stdio.h>
#include <stdbool.h>
#include "mtk_c.h"
#include "csys68k.c"

typedef enum{
    ACTION_NONE,
    ACTION_ATTACK,
    ACTION_GUARD,
    ACTION_THROW,
    ACTION_STEP
} Action_Type;

typedef struct{
    int hp;
    Action_Type current_action;
    int cooldown_ticks;
    bool guarding;
    bool stepping;
    bool action_ready;
    SEMAPHORE_ID_TYPE action_semaphore;
} PlayerState;

PlayerState player1;
PlayerState player2;

//------------------------------------------
// 初期化部分
//------------------------------------------
#define SEM_PLAYER1 0
#define SEM_PLAYER2 1
void game_init(){
    /*プレイヤーの初期化*/
    player_init(&player1, SEM_PLAYER1);
    player_init(&player2, SEM_PLAYER2);
    /*その他ゲーム初期化処理*/

}

void player_init(PlayerState* player, SEMAPHORE_ID_TYPE sem_id){
    player->hp = 100;
    player->current_action = ACTION_NONE;
    player->cooldown_ticks = 0;
    player->guarding = false;
    player->stepping = false;
    player->action_ready = true;
    player->action_semaphore = sem_id; // 初期セマフォID
}

//------------------------------------------
// プレイヤー入力タスク
//------------------------------------------
void p1_input_task(){
    player_input_task(&player1, 3); // fd 3: UART1
}
void p2_input_task(){
    player_input_task(&player2, 4); // fd 4: UART2
}


void player_input_task(PlayerState* player, int fd){
    while(1){
        if(player->cooldown_ticks > 0){
            player->cooldown_ticks--;
            skip_mt();
            continue;
        }

        char ch;
        if(read(fd, &ch, 1) == -1){
            skip_mt();
            continue;
        }
        Action_Type action = ACTION_NONE;
        switch (ch) {
            case 'a': action = ACTION_ATTACK; break;
            case 'd': action = ACTION_GUARD;  break;
            case 't': action = ACTION_THROW;  break;
            case 's': action = ACTION_STEP;   break;
            default: {skip_mt(); continue;} // 無効な入力
        }
        P(player->action_semaphore);
        // 入力を反映
        player->current_action = action;
        player->action_ready = true;

        // クールダウン設定
        switch (action) {
            case ACTION_ATTACK: player->cooldown_ticks = 10; break;
            case ACTION_GUARD:  player->cooldown_ticks = 8;  break;
            case ACTION_THROW:  player->cooldown_ticks = 12; break;
            case ACTION_STEP:   player->cooldown_ticks = 20; break;
            default: break;
        }

        // 状態フラグ
        player->guarding = (action == ACTION_GUARD);
        player->stepping = (action == ACTION_STEP);

        V(player->action_semaphore);
        // 入力後すぐにスケジューラを回す
        skip_mt();
    }
}

void game_logic_task() {
    while (1) {
        // プレイヤーの状態をローカルにコピー
        PlayerState p1_copy, p2_copy;

        // --- プレイヤー1の状態を読み取り ---
        P(player1.action_semaphore);
        p1_copy = player1;
        V(player1.action_semaphore);

        // --- プレイヤー2の状態を読み取り ---
        P(player2.action_semaphore);
        p2_copy = player2;
        V(player2.action_semaphore);

        // 両方の入力が揃っていたら処理
        if (p1_copy.action_ready && p2_copy.action_ready) {
            // アクション解決（ローカルコピーを使って判定）
            resolve_actions(&p1_copy, &p2_copy);

            // 結果を本体に書き戻す（セマフォ保護）
            P(player1.action_semaphore);
            player1.hp = p1_copy.hp;
            player1.guarding = false;
            player1.stepping = false;
            player1.action_ready = false;
            V(player1.action_semaphore);

            P(player2.action_semaphore);
            player2.hp = p2_copy.hp;
            player2.guarding = false;
            player2.stepping = false;
            player2.action_ready = false;
            V(player2.action_semaphore);
        }

        // 少し待ってから次のチェックへ（1ティック分）
        wait_ticks(1);
    }
}


int main(){
    
    return 0;
}
