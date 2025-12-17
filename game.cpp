#include <stdio.h>
#include <unistd.h>

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
} PlayerState;

void player_init(PlayerState* player, int initial_hp){
    player->hp = initial_hp;
    player->current_action = ACTION_NONE;
    player->cooldown_ticks = 0;
    player->guarding = false;
    player->stepping = false;
    player->action_ready = true;
}

void player_input_task(PlayerState* player, int fd){
    while(1){
        if(player->cooldown_ticks > 0){
            player->cooldown_ticks--;
            continue;
        }

        char ch;
        if(read(fd, &ch, 1) <= 0){
            continue;
        }
        Action_Type action = ACTION_NONE;
        switch (ch) {
            case 'a': action = ACTION_ATTACK; break;
            case 'd': action = ACTION_GUARD;  break;
            case 't': action = ACTION_THROW;  break;
            case 's': action = ACTION_STEP;   break;
            default: continue; // 無効な入力
        }
    }

    
}

int main(){
    
    return 0;
}
