#include<iostream>
#include<vector>
#include<algorithm>
#include<map>
#include<set>
#include<queue>
using namespace std;

//キャラ選択
enum{
    RYU,
    LUKE,
    BLANKA,
    CHUNLI,
    DHALSIM,
    DEEJAY,
    GUILE,
    E_HONDA,
    JAMIE,
    JURI,
    KEN,
    KIMBERLY,
    MANON,
    MARISA,
    JP,
    ZANGIEF,
    LILY,
    CAMMY,
    RASHID,
    AKI,
    ED,
    AKUMA,
    VEGA,
    TERRY,
    MAI
};
//技名
enum{
    LP,MP,HP,//弱中強P
    LK,MK,HK,//弱中強K
    LP2,MP2,HP2,//屈弱中強P
    LK2,MK2,HK2,//屈弱中強K
    LP236,MP236,HP236,//波動P
    LP214,MP214,HP214,//竜巻P
    LP623,MP623,HP623,//昇竜P
    LK236,MK236,HK236,//波動K
    LK214,MK214,HK214,//竜巻K
    LK623,MK623,HK623,//昇竜K
    LP228,MP228,HP228,//下溜めP
    LK228,MK228,HK228,//下溜めK
    LP236236,MP236236,HP236236,//真空波動P
    LP214214,MP214214,HP214214,//真空竜巻P
    LK236236,MK236236,HK236236,//真空波動K
    LK214214,MK214214,HK214214,//真空竜巻K
};

//キャラの技の情報を格納するクラス
//技の情報はダメージ、発生、持続、硬直、硬直差、ヒットストップを格納する
//ダメージはint型、発生はint型、持続はint型、硬直はint型、硬直差はint型、ヒットストップはint型である
//技の情報を格納するクラスはweaponとする

class weapon{
public:
    int damage; //ダメージ
    int start_up; //発生
    int active; //持続
    int recovery; //硬直
    int frame_advantage; //硬直差
    int hit_stop; //ヒットストップ

    weapon(int d, int s, int a, int r, int f, int h) : damage(d), start_up(s), active(a), recovery(r), frame_advantage(f), hit_stop(h) {}
};

class character{
    public:
    vector<weapon> weapons[48]; //技の情報を格納するベクター
    void set_weapon(int index, int d, int s, int a, int r, int f, int h) {
        weapons[index].push_back(weapon(d, s, a, r, f, h)); //技の情報を格納する
    }
};
character ryu, luke, blanka, chunli, dhalsim, deejay, guile, e_honda, jamie, juri, ken, kimberly, manon, marisa, jp, zangief, lily, cammy, rashid, aki, ed, akuma, vega, terry, mai; //キャラのインスタンスを作成
int main() {
    ryu.set_weapon(LP, 30, 4, 2, 10, 0, 0); //リュウの弱Pの情報を格納
    cout << "Ryu's LP damage: " << ryu.weapons[LP][0].damage << endl; //リュウの弱Pのダメージを出力
    return 0;
}