#include <stdio.h>
#include <stdbool.h>
#include "mtk_c.h"
#include "csys68k.c"

extern void skipmt();
extern int inkey(int port);

typedef enum{
    ACTION_NONE,
    ACTION_ATTACK,
    ACTION_GUARD,
    ACTION_THROW,
    ACTION_STEP
} Action_Type;

// 前方宣言（このファイル内の関数）
typedef struct
{
    int hp;
    Action_Type current_action;
    int cooldown_ticks;
    bool guarding;
    bool stepping;
    bool action_ready;
    SEMAPHORE_ID_TYPE action_semaphore;
} PlayerState;

void player_init(PlayerState *player, SEMAPHORE_ID_TYPE sem_id);
void player_input_task(PlayerState *player, int fd, const char *player_name);
void enqueue_event(const char *event_msg);
void resolve_actions(PlayerState *p1, PlayerState *p2);
extern bool game_over;
int inkey(int port);

// PlayerState は前方宣言済み

PlayerState player1;
PlayerState player2;
char event_queue[1024][128];
int event_queue_start = 0;
int event_queue_end = 0;
//------------------------------------------
// 初期化部分
//------------------------------------------
#define SEM_PLAYER1 0
#define SEM_PLAYER2 1
#define SEM_EVENT_Q 2

void game_init()
{
    /*プレイヤーの初期化*/
    player_init(&player1, SEM_PLAYER1);
    player_init(&player2, SEM_PLAYER2);
    /*その他ゲーム初期化処理*/
    game_over = false;
}

void player_init(PlayerState *player, SEMAPHORE_ID_TYPE sem_id)
{
    player->hp = 100;
    player->current_action = ACTION_NONE;
    player->cooldown_ticks = 0;
    player->guarding = false;
    player->stepping = false;
    player->action_ready = false;
    player->action_semaphore = sem_id; // 初期セマフォID
}

//------------------------------------------
// プレイヤー入力部分
//------------------------------------------
void p1_input_task()
{
    player_input_task(&player1, 3, "1P"); // fd 3: UART1
}
void p2_input_task()
{
    player_input_task(&player2, 4, "2P"); // fd 4: UART2
}

int fd2port(int fd)
{
    switch (fd)
    {
    case 3:
        return 0; // UART1
    case 4:
        return 1; // UART2
    default:
        return -1; // 不明
    }
}

void player_input_task(PlayerState *player, int fd, const char *player_name)
{
    int port = fd2port(fd);
    while (!game_over)
    {
        if (player->cooldown_ticks > 0)
        {
            player->cooldown_ticks--;
            skipmt();
            continue;
        }

        char ch = inkey(port);
        if (ch == -1)
        {
            skipmt();
            continue;
        }
        Action_Type action = ACTION_NONE;
        const char *action_str = "入力なし";
        switch (ch)
        {
        case 'a':
            action = ACTION_ATTACK;
            action_str = "攻撃";
            break;
        case 'd':
            action = ACTION_GUARD;
            action_str = "ガード";
            break;
        case 't':
            action = ACTION_THROW;
            action_str = "投げ";
            break;
        case 's':
            action = ACTION_STEP;
            action_str = "ステップ";
            break;
        default:
        {
            skipmt();
            continue;
        } // 無効な入力
        }

        // 処理待ちならスキップ
        if (player->action_ready == true)
        {
            // すでに入力済み
            skipmt();
            continue;
        }
        // 入力を反映
        P(player->action_semaphore);
        player->current_action = action;
        player->action_ready = true;

        // クールダウン設定
        switch (action)
        {
        case ACTION_ATTACK:
            player->cooldown_ticks = 10;
            break;
        case ACTION_GUARD:
            player->cooldown_ticks = 8;
            break;
        case ACTION_THROW:
            player->cooldown_ticks = 12;
            break;
        case ACTION_STEP:
            player->cooldown_ticks = 20;
            break;
        default:
            break;
        }

        // 状態フラグ
        player->guarding = (action == ACTION_GUARD);
        player->stepping = (action == ACTION_STEP);

        V(player->action_semaphore);
        // 入力後すぐにスケジューラを回す

        char log[128];
        snprintf(log, sizeof(log), "%sが%sを選択！", player_name, action_str);
        enqueue_event(log);

        skipmt();
    }
}

//------------------------------------------
// ゲームロジック部分
//------------------------------------------
bool game_over = false;
#define WAIT_TICKS 10

void game_logic_task()
{
    while (!game_over)
    {
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

        // 片方の入力が揃っていたら処理
        if (p1_copy.action_ready || p2_copy.action_ready)
        {
            int waited = 0;
            // もう片方の入力を少し待つ
            while (waited < WAIT_TICKS)
            {
                P(player1.action_semaphore);
                p1_copy = player1;
                V(player1.action_semaphore);

                P(player2.action_semaphore);
                p2_copy = player2;
                V(player2.action_semaphore);

                if (p1_copy.action_ready && p2_copy.action_ready)
                    break;
                waited++;
            }

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

            // 勝敗チェック
            if (player1.hp <= 0 && player2.hp <= 0)
            {
                enqueue_event("引き分け！");
                game_over = true;
            }
            else if (player1.hp <= 0)
            {
                enqueue_event("P2の勝利！");
                game_over = true;
            }
            else if (player2.hp <= 0)
            {
                enqueue_event("P1の勝利！");
                game_over = true;
            }
        }

        // 少し待ってから次のチェックへ（1ティック分）
        skipmt();
    }
    enqueue_event("ゲーム終了！");
}

void resolve_actions(PlayerState *p1, PlayerState *p2)
{
    int p1_damage = 0;
    int p2_damage = 0;

    // --- P1の行動がP2に与える影響 ---
    switch (p1->current_action)
    {
    case ACTION_ATTACK:
        if (p2->stepping)
        {
            enqueue_event("P2がステップでP1の攻撃を回避！");
        }
        else if (p2->guarding)
        {
            p2_damage += 5;
            enqueue_event("P1の攻撃！P2はガードして5ダメージ！");
        }
        else
        {
            p2_damage += 10;
            enqueue_event("P1の攻撃！P2に10ダメージ！");
        }
        break;
    case ACTION_THROW:
        if (p2->stepping)
        {
            enqueue_event("P2がステップでP1の投げを回避！");
        }
        else if (p2->guarding)
        {
            p2_damage += 15;
            enqueue_event("P1の投げ成功！P2のガードを崩した！");
        }
        else
        {
            p2_damage += 10;
            enqueue_event("P1の投げ！P2に10ダメージ！");
        }
        break;
    default:
        break;
    }

    // --- P2の行動がP1に与える影響 ---
    switch (p2->current_action)
    {
    case ACTION_ATTACK:
        if (p1->stepping)
        {
            enqueue_event("P1がステップでP2の攻撃を回避！");
        }
        else if (p1->guarding)
        {
            p1_damage += 5;
            enqueue_event("P2の攻撃！P1はガードして5ダメージ！");
        }
        else
        {
            p1_damage += 10;
            enqueue_event("P2の攻撃！P1に10ダメージ！");
        }
        break;
    case ACTION_THROW:
        if (p1->stepping)
        {
            enqueue_event("P1がステップでP2の投げを回避！");
        }
        else if (p1->guarding)
        {
            p1_damage += 15;
            enqueue_event("P2の投げ成功！P1のガードを崩した！");
        }
        else
        {
            p1_damage += 10;
            enqueue_event("P2の投げ！P1に10ダメージ！");
        }
        break;
    default:
        break;
    }

    // --- 最後にHPをまとめて減らす ---
    p1->hp -= p1_damage;
    p2->hp -= p2_damage;
}

//------------------------------------------
// 描画部分
//------------------------------------------

void enqueue_event(const char *event_msg)
{
    P(SEM_EVENT_Q);
    // イベントキューにメッセージを追加
    snprintf(event_queue[event_queue_end], 128, "%s", event_msg);
    event_queue_end = (event_queue_end + 1) % 1024;
    // キューがいっぱいになったら古いイベントを上書き
    if (event_queue_end == event_queue_start)
    {
        event_queue_start = (event_queue_start + 1) % 1024;
    }
    V(SEM_EVENT_Q);
}

void draw_task()
{
    int idx = event_queue_start;
    while (!game_over)
    {
        // イベントキューの表示
        P(SEM_EVENT_Q);
        while (idx != event_queue_end)
        {
            printf("%s\r\n", event_queue[idx]);
            idx = (idx + 1) % 1024;
        }
        V(SEM_EVENT_Q);

        for (int i = 0; i < 10000; i++)
            ; // 簡易ディレイ
        skipmt();
    }
}

/*void clear_screen() {
    // 画面クリアの実装（環境依存）
    printf("\033[2J\033[H"); // ANSIエスケープシーケンスでクリア
}
void refresh_screen() {
    // 画面更新の実装（環境依存）
    fflush(stdout);
}*/

int main()
{
    init_kernel();
    game_init();
    set_task(p1_input_task);
    set_task(p2_input_task);
    set_task(game_logic_task);
    set_task(draw_task);
    begin_sch();
    return 0;
}
