#include <stdio.h>
#include <iostream>
#include <cassert>
#include <queue>
#include <algorithm>
#include <string.h>
#include <unordered_map>

using namespace std;
typedef long long ll;

const int MAX_TURN = 500; // ゲームの最大ターン数
const int FIELD_WIDTH = 10; // フィールドの横幅
const int FIELD_HEIGHT = 16; // フィールドの縦幅
const int WIDTH = 2 + FIELD_WIDTH + 2; // 余分な領域も含めたフィールドの横幅
const int HEIGHT = FIELD_HEIGHT + 3; // 余分な領域も含めたフィールドの縦幅
const int DANGER_LINE = 16; // 危険ライン

const char DELETED_SUM = 10; // 消滅のために作るべき和の値

const char EMPTY = 0; // 空のグリッド
const char OJAMA = 11; // お邪魔ブロック

const int WIN = 99999;

int BEAM_WIDTH = 1000;
int SEARCH_DEPTH = 8;

/**
 * 乱数生成器
 */
unsigned long long xor128(){
  static unsigned long long rx=123456789, ry=362436069, rz=521288629, rw=88675123;
  unsigned long long rt = (rx ^ (rx<<11));
  rx=ry; ry=rz; rz=rw;
  return (rw=(rw^(rw>>19))^(rt^(rt>>8)));
}

struct Pack {
  int t[9];

  Pack () {
    memset(t, -1, sizeof(t));
  }
};

char g_myField[WIDTH][HEIGHT]; // 自フィールド
int g_enemyField[WIDTH][HEIGHT]; // 敵フィールド

int g_myPutPackLine[WIDTH]; // 次にブロックを設置する高さを保持する配列
int g_enemyPutPackLine[WIDTH]; // 次にブロックを設置する高さを保持する配列

int g_packDeleteCount[WIDTH][HEIGHT];

int g_maxHeight;
int g_minHeight;
int g_beforeTime;
int g_myOjamaStock;

bool g_chain;
bool g_enemyPinch;

ll g_zoblishField[FIELD_WIDTH][FIELD_HEIGHT][12]; // zoblish hash生成用の乱数テーブル

Pack g_packs[MAX_TURN]; // パック一覧

struct Command {
  int pos;
  int rot;

  Command (int pos = 0, int rot = 0) {
    this->pos = pos;
    this->rot = rot;
  }
};

struct Node {
  int value;
  bool chain;
  Command command;
  char field[WIDTH][HEIGHT];

  Node () {
    this->value = 0;
  }

  ll hashCode() {
    ll hash = 0;

    for (int x = 0; x < FIELD_WIDTH; x++) {
      for (int y = 0; y < FIELD_HEIGHT; y++) {
        int num = this->field[x+2][y];
        if (num == EMPTY) continue;
        hash ^= g_zoblishField[x][y][num];
      }
    }

    return hash;
  }

  bool operator >(const Node &n) const {
    return value < n.value;
  }
};

class Codevs {
public:
  /**
   * 1. ゲーム開始時の入力情報を読み込む
   * 2. フィールド情報を初期化しておく
   * 3. zoblish hash用の乱数テーブル生成
   * 4. パック情報の読み込み
   */
  void init() {
    fprintf(stderr,"init =>\n");
    int _w, _h, _t, _s, _n;
    cin >> _w >> _h >> _t >> _s >> _n;

    memset(g_myField, EMPTY, sizeof(g_myField));
    memset(g_enemyField, EMPTY, sizeof(g_enemyField));

    for (int x = 0; x < FIELD_WIDTH; x++) {
      for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int i = 0; i < 12; i++) {
          g_zoblishField[x][y][i] = xor128();
        }
      }
    }

    g_beforeTime = 180000;
    g_enemyPinch = false;

    readPackInfo();
  }

  /**
   * パック情報を読み込む
   */
  void readPackInfo() {
    fprintf(stderr,"readPackInfo =>\n");
    int t0, t1, t2, t3, t4, t5, t6, t7, t8;
    string _end_;

    for (int i = 0; i < MAX_TURN; i++) {
      Pack pack;
      cin >> t0 >> t1 >> t2;
      cin >> t3 >> t4 >> t5;
      cin >> t6 >> t7 >> t8;

      pack.t[0] = t0; pack.t[1] = t1; pack.t[2] = t2;
      pack.t[3] = t3; pack.t[4] = t4; pack.t[5] = t5;
      pack.t[6] = t6; pack.t[7] = t7; pack.t[8] = t8;

      g_packs[i] = pack;

      cin >> _end_;
    }
  }

  /**
   * AIのメインの処理部分
   *
   * @param [int] turn 現在のターン数
   */
  void run(int turn) {
    readTurnInfo();
    updatePutPackLine();

    if (g_myOjamaStock > 0) {
      fillOjama(turn, g_myOjamaStock);
    }

    Command command = getBestCommand(turn);
    cout << command.pos-2 << " " << command.rot << endl;
    fflush(stderr);

    if (g_myOjamaStock > 0) {
      cleanOjama(turn);
    }
  }

  /**
   * 一番良い操作を取得する
   *
   * @param [int] turn 今現在のターン
   */
  Command getBestCommand(int turn) {
    Node root;
    memcpy(root.field, g_myField, sizeof(g_myField));
    Command bestCommand;
    int maxValue = -9999;

    queue<Node> que;
    que.push(root);

    unordered_map<ll, bool> checkNodeList;

    for (int depth = 0; depth < SEARCH_DEPTH; depth++) {
      priority_queue<Node, vector<Node>, greater<Node> > pque;
      Pack pack = g_packs[turn + depth];

      while (!que.empty()) {
        Node node = que.front(); que.pop();
        memcpy(g_myField, node.field, sizeof(node.field));
        updatePutPackLine();

        for (int x = 0; x < WIDTH-2; x++) {
          for (int rot = 0; rot < 4; rot++) {
            putPack(x, rot, pack);

            if (isValidField()) {
              Node cand;
              cand.value = simulate(depth);
              cand.chain = g_chain;

              if (depth == 0) {
                cand.command = Command(x, rot);
              } else {
                cand.command = node.command;
              }

              memcpy(cand.field, g_myField, sizeof(g_myField));
              pque.push(cand);
            }

            memcpy(g_myField, node.field, sizeof(node.field));
            updatePutPackLine();
          }
        }
      }

      if (depth < SEARCH_DEPTH) {
        for (int j = 0; j < BEAM_WIDTH && !pque.empty(); j++) {
          Node node = pque.top(); pque.pop();

          if (maxValue < node.value) {
            maxValue = node.value;
            bestCommand = node.command;
          }

          ll hash = node.hashCode();

          if (!checkNodeList[hash] && !(depth > 0 && node.chain)) {
            checkNodeList[hash] = true;
            que.push(node);
          } else {
            j--;
          }
        }
      }
    }

    return bestCommand;
  }

  /**
   * パックの落下処理
   */
  void fallPack() {
    for (int x = 2; x < WIDTH-2; x++) {
      int fallCnt = 0;

      for (int y = 0; y < g_myPutPackLine[x]; y++) {
        if (g_myField[x][y] == EMPTY) {
          fallCnt++;
        } else if (fallCnt > 0) {
          g_myField[x][y-fallCnt] = g_myField[x][y];
          g_myField[x][y] = EMPTY;
        }
      }
    }
  }

  /**
   * パックを設置する処理
   *
   * @param [int] x 設置するx座標
   * @param [int] rot 回転数
   * @param [Pack] pack パック情報
   */
  void putPack(int x, int rot, const Pack &pack) {
    switch (rot) {
      case 0:
        putLinePack(  x, pack.t[6], pack.t[3], pack.t[0]);
        putLinePack(x+1, pack.t[7], pack.t[4], pack.t[1]);
        putLinePack(x+2, pack.t[8], pack.t[5], pack.t[2]);
        break;
      case 1:
        putLinePack(  x, pack.t[8], pack.t[7], pack.t[6]);
        putLinePack(x+1, pack.t[5], pack.t[4], pack.t[3]);
        putLinePack(x+2, pack.t[2], pack.t[1], pack.t[0]);
        break;
      case 2:
        putLinePack(  x, pack.t[2], pack.t[5], pack.t[8]);
        putLinePack(x+1, pack.t[1], pack.t[4], pack.t[7]);
        putLinePack(x+2, pack.t[0], pack.t[3], pack.t[6]);
        break;
      case 3:
        putLinePack(  x, pack.t[0], pack.t[1], pack.t[2]);
        putLinePack(x+1, pack.t[3], pack.t[4], pack.t[5]);
        putLinePack(x+2, pack.t[6], pack.t[7], pack.t[8]);
        break;
      default:
        assert(false);
    }
  }

  /**
   * 指定したx座標にパックの一部を落とす (下のような感じで落とす)
   *
   *    t0 t1 t2
   *
   * @param [int] x x座標
   * @param [int] t0 パックのブロックの値
   * @param [int] t1 パックのブロックの値
   * @param [int] t2 パックのブロックの値
   */
  void putLinePack(int x, int t0, int t1, int t2) {
    int y = g_myPutPackLine[x];

    assert(t0 >= 0);
    assert(t1 >= 0);
    assert(t2 >= 0);

    if (t0 != EMPTY) { g_myField[x][y] = t0; y++; }
    if (t1 != EMPTY) { g_myField[x][y] = t1; y++; }
    if (t2 != EMPTY) { g_myField[x][y] = t2; y++; }

    g_myPutPackLine[x] = y;
  }

  /**
   * パックにお邪魔を埋め込む
   *
   * TODO: 複数ターンにまたがるお邪魔についても対応を考える
   */
  void fillOjama(int turn, int ojamaStock) {
    for (int t = turn; t <= min(MAX_TURN-1, (turn + SEARCH_DEPTH)) && ojamaStock > 0; t++) {
      Pack *pack = &g_packs[t];

      for (int i = 0; i < 9 && ojamaStock > 0; i++) {
        if (pack->t[i] == EMPTY) {
          pack->t[i] = OJAMA;
          ojamaStock--;
        }
      }
    }
  }

  /**
   * ブロックからお邪魔を取り除く
   */
  void cleanOjama(int turn) {
    for (int t = turn; t <= min(MAX_TURN-1, (turn + SEARCH_DEPTH)); t++) {
      Pack *pack = &g_packs[t];

      for (int i = 0; i < 9; i++) {
        if (pack->t[i] == OJAMA) {
          pack->t[i] = EMPTY;
        }
      }
    }
  }

  /**
   * 有効なフィールドかどうかを調べる
   */
  bool isValidField() {
    if (g_myPutPackLine[0] > 0) return false;
    if (g_myPutPackLine[1] > 0) return false;
    if (g_myPutPackLine[12] > 0) return false;
    if (g_myPutPackLine[13] > 0) return false;

    for (int x = 2; x < WIDTH-2; x++) {
      if (g_myPutPackLine[x] >= DANGER_LINE) return false;
    }

    return true;
  }

  /**
   * 連鎖処理のシミュレーションを行う
   *
   * @param [int] 評価値
   */
  int simulate(int depth) {
    int chainCnt = 0;
    int value = 0;
    updatePutPackLine();
    g_chain = false;
    int score = 0;

    while (true) {
      int deleteCount = chainPack();

      if (deleteCount > 0) {
        fallPack();
        chainCnt++;
      }

      score += floor(pow(1.3, chainCnt) * (deleteCount/2));

      if (depth > 0) {
        value += floor(pow(1.4, chainCnt) * (deleteCount/2));
      } else if (chainCnt >= 13) {
        value += 3 * floor(pow(1.4, chainCnt) * (deleteCount/2));
      }

      if (deleteCount == 0) break;
    }

    if (chainCnt >= 4) {
      g_chain = true;
    }
    if (score >= 300 || (g_enemyPinch && score >= 60)) {
      value += WIN;
    }

    return value;
  }

  /**
   * 連鎖判定を行う
   *
   * @return [int] 連鎖カウント
   */
  int chainPack() {
    memset(g_packDeleteCount, 0, sizeof(g_packDeleteCount));

    for (int y = 0; y < g_maxHeight; y++) {
      deleteCheckHorizontal(y);
    }
    for (int y = 0; y < HEIGHT; y++) {
      deleteCheckDiagonalRightUp(y, 2);
      deleteCheckDiagonalRightDown(y, 2);
    }
    for (int x = 2; x < WIDTH-2; x++) {
      deleteCheckVertical(x);
      deleteCheckDiagonalRightUp(0, x);
      deleteCheckDiagonalRightDown(HEIGHT-1, x);
    }

    int deleteCnt = deletePack();
    return deleteCnt;
  }

  /**
   * 連鎖判定で削除となったパックを消す
   *
   * @return [int] 削除カウント
   */
  int deletePack() {
    int deleteCnt = 0;
    int cnt = 0;

    for (int x = 2; x < WIDTH-2; x++) {
      for (int y = 0; y < g_maxHeight; y++) {
        cnt = g_packDeleteCount[x][y];

        if (cnt > 0) {
          g_myField[x][y] = EMPTY;
          deleteCnt += cnt;
        }
      }
    }

    return deleteCnt;
  }

  /**
   * 横のラインの削除判定を行う
   *
   * @param [int] y チェックするy座標
   */
  void deleteCheckHorizontal(int y) {
    int fromX = 2;
    int toX = 2;
    char sum = g_myField[toX][y];

    while (toX < WIDTH-2) {
      if (sum < DELETED_SUM) {
        toX++;

        if (toX >= WIDTH-2) break;

        if (sum == 0) {
          fromX = toX;
        }

        char num = g_myField[toX][y];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
          fromX = toX;
        } else {
          sum += num;
        }
      } else {
        assert(g_myField[fromX][y] != EMPTY);
        sum -= g_myField[fromX][y];
        fromX++;

        if (fromX > toX) {
          toX = fromX;
        }
      }

      if (sum == DELETED_SUM) {
        for (int x = fromX; x <= toX; x++) {
          g_packDeleteCount[x][y]++;
        }
      }
    }
  }

  /**
   * 縦のラインの削除判定を行う
   *
   * @param [int] x チェックするx座標
   */
  void deleteCheckVertical(int x) {
    int fromY = 0;
    int toY = 0;
    char sum = g_myField[x][toY];

    while (toY < HEIGHT) {
      if (sum < DELETED_SUM) {
        toY++;
        if (toY >= HEIGHT) break;

        if (sum == 0) {
          fromY = toY;
        }

        char num = g_myField[x][toY];
        if (num == EMPTY) break;

        if (num == OJAMA) {
          sum = 0;
          fromY = toY;
        } else {
          sum += num;
        }
      } else {
        sum -= g_myField[x][fromY];
        fromY++;

        if (fromY > toY) {
          toY = fromY;
        }
      }

      if (sum == DELETED_SUM) {
        for (int y = fromY; y <= toY; y++) {
          g_packDeleteCount[x][y]++;
        }
      }
    }
  }

  /**
   * 斜めのラインの削除判定を行う (右上に進む)
   *
   * @param [int] sy チェックするy座標
   * @param [int] sx チェックするx座標
   */
  void deleteCheckDiagonalRightUp(int sy, int sx) {
    int fromY = sy;
    int fromX = sx;
    int toY = sy;
    int toX = sx;
    char sum = g_myField[toX][toY];

    while (toX < WIDTH-2 && toY < g_maxHeight) {
      assert(fromX <= toX);

      if (sum < DELETED_SUM) {
        toY++;
        toX++;

        if (toX >= WIDTH-2 || toY >= HEIGHT) break;

        if (sum == 0) {
          fromY = toY;
          fromX = toX;
        }

        char num = g_myField[toX][toY];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
          fromY = toY;
          fromX = toX;
        } else {
          sum += num;
        }
      } else {
        assert(g_myField[fromX][fromY] != EMPTY);
        sum -= g_myField[fromX][fromY];
        fromY++;
        fromX++;

        if (fromX > toX) {
          toX = fromX;
          toY = fromY;
        }
      }

      if (sum == DELETED_SUM) {
        int i = 0;
        for (int x = fromX; x <= toX; x++) {
          g_packDeleteCount[x][fromY+i]++;
          i++;
        }
      }
    }
  }

  /**
   * 連鎖判定
   *
   * @param [int] sy チェックするy座標
   * @param [int] sx チェックするx座標
   */
  void deleteCheckDiagonalRightDown(int sy, int sx) {
    int fromY = sy;
    int fromX = sx;
    int toY = sy;
    int toX = sx;
    char sum = g_myField[toX][toY];

    while (toX < WIDTH && toY >= 0) {
      if (sum < DELETED_SUM) {
        toY--;
        toX++;

        if (toX >= WIDTH-2 || toY < 0) break;

        if (sum == 0) {
          fromY = toY;
          fromX = toX;
        }

        int num = g_myField[toX][toY];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
          fromY = toY;
          fromX = toX;
        } else {
          sum += num;
        }
      } else {
        sum -= g_myField[fromX][fromY];
        fromY--;
        fromX++;

        if (fromX > toX) {
          toY = fromY;
          toX = fromX;
        }
      }

      if (sum == DELETED_SUM) {
        int i = 0;
        for (int x = fromX; x <= toX; x++) {
          g_packDeleteCount[x][fromY-i]++;
          i++;
        }
      }
    }
  }

  /**
   * ターン毎の情報を読み込む
   */
  void readTurnInfo() {
    string _end_;

    // [現在のターン数]
    int turn;
    cin >> turn;

    // [自分の残り思考時間。単位はミリ秒]
    int myRemainTime;
    cin >> myRemainTime;

    fprintf(stderr,"%d: myRemainTime = %d, use time = %d\n", turn, myRemainTime, g_beforeTime - myRemainTime);
    g_beforeTime = myRemainTime;

    // [自分のお邪魔ストック]
    cin >> g_myOjamaStock;


    // [前のターン終了時の自分のフィールド情報]
    int t;
    for (int y = 0; y < FIELD_HEIGHT; y++) {
      for (int x = 2; x < WIDTH-2; x++) {
        cin >> t;
        g_myField[x][FIELD_HEIGHT-y-1] = t;
      }
    }

    cin >> _end_;

    // [相手のお邪魔ストック]
    int enemyOjamaStock;
    cin >> enemyOjamaStock;

    int ojamaCnt = 0;

    // [前のターン終了時の相手のフィールド情報]
    for (int y = 0; y < FIELD_HEIGHT; y++) {
      for (int x = 2; x < WIDTH-2; x++) {
        cin >> t;
        g_enemyField[x][FIELD_HEIGHT-y-1] = t;
        if (t == OJAMA) {
          ojamaCnt++;
        }
      }
    }

    if (ojamaCnt >= 40) {
      g_enemyPinch = true;
    }

    cin >> _end_;
  }

  /**
   * ブロックを落下させる時に落下させる位置を更新する
   */
  void updatePutPackLine() {
    g_maxHeight = 0;
    g_minHeight = HEIGHT;

    for (int x = 0; x < WIDTH; x++) {
      setPutPackLine(x);
    }
  }

  /**
   * @param [int] x x座標の値
   */
  void setPutPackLine(int x) {
    int y = 0;

    while (g_myField[x][y] != EMPTY && y < HEIGHT-1) {
      y++;
    }

    g_myPutPackLine[x] = y;
    g_maxHeight = max(g_maxHeight, y);
    g_minHeight = min(g_minHeight, y);
  }

  /**
   * フィールドを表示する(デバッグ用)
   */
  void showField() {
    fprintf(stderr,"\n");
    for (int x = 0; x < WIDTH; x++) {
      for (int y = 0; y < HEIGHT; y++) {
        if (g_myField[x][y] == 11) {
          fprintf(stderr,"B");
        } else {
          fprintf(stderr,"%d", g_myField[x][y]);
        }
      }
      fprintf(stderr," %d\n", g_myPutPackLine[x]);
    }
    fprintf(stderr,"\n");
  }
};

int main() {
  Codevs cv;

  cout << "siman" << endl;

  cv.init();

  for (int i = 0; i < MAX_TURN; i++) {
    cv.run(i);
  }

  return 0;
}
