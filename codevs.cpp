#include <stdio.h>
#include <iostream>
#include <cassert>
#include <queue>
#include <algorithm>
#include <string.h>

using namespace std;

const int MAX_TURN = 500; // ゲームの最大ターン数
const int FIELD_WIDTH = 10; // フィールドの横幅
const int FIELD_HEIGHT = 16; // フィールドの縦幅
const int WIDTH = 2 + FIELD_WIDTH + 2; // 余分な領域も含めたフィールドの横幅
const int HEIGHT = FIELD_HEIGHT + 3; // 余分な領域も含めたフィールドの縦幅
const int DANGER_LINE = 16; // 危険ライン

const int DELETED_SUM = 10; // 消滅のために作るべき和の値

const char EMPTY = 0; // 空のグリッド
const char OJAMA = 11; // お邪魔ブロック

int BEAM_WIDTH = 100;
int SEARCH_DEPTH = 10;

struct Pack {
  char t[9];
};

char g_myField[WIDTH][HEIGHT]; // 自フィールド
char g_enemyField[WIDTH][HEIGHT]; // 敵フィールド
char g_tempField[WIDTH][HEIGHT]; // 保存用のフィールド

int g_myPutPackLine[WIDTH]; // 次にブロックを設置する高さを保持する配列
int g_enemyPutPackLine[WIDTH]; // 次にブロックを設置する高さを保持する配列
int g_tempPutPackLine[WIDTH]; // 保存用の配列

int g_packDeleteCount[HEIGHT][WIDTH];

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
  char field[HEIGHT][WIDTH];
  int value;

  Node () {
    this->value = 0;
  }

  bool operator >(const Node &n) const {
    return value < n.value;
  }
};

class Codevs {
public:
  /**
   * ゲーム開始時の入力情報を読み込む
   * フィールド情報を初期化しておく
   */
  void init() {
    fprintf(stderr,"init =>\n");
    int _w, _h, _t, _s, _n;
    cin >> _w >> _h >> _t >> _s >> _n;

    memset(g_myField, EMPTY, sizeof(g_myField));
    memset(g_enemyField, EMPTY, sizeof(g_enemyField));

    readPackInfo();
  }

  /**
   * パック情報を読み込む
   */
  void readPackInfo() {
    fprintf(stderr,"readPackInfo =>\n");
    char t0, t1, t2, t3, t4, t5, t6, t7, t8;
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

    Command cmd = getBestCommand();
    cout << cmd.pos << " " << cmd.rot << endl;
  }

  /**
   * 一番良い操作を取得する
   */
  Command getBestCommand() {
    Command bestCommand;

    return bestCommand;
  }

  /**
   * パックの落下処理
   */
  void fallPack() {
    for (int x = 0; x < WIDTH; x++) {
      int fallCnt = 0;

      for (int y = 1; y < HEIGHT; y++) {
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
        putLinePack(  x, pack.t[0], pack.t[3], pack.t[6]);
        putLinePack(x+1, pack.t[1], pack.t[4], pack.t[7]);
        putLinePack(x+2, pack.t[2], pack.t[5], pack.t[8]);
        break;
      case 1:
        putLinePack(  x, pack.t[6], pack.t[7], pack.t[8]);
        putLinePack(x+1, pack.t[3], pack.t[4], pack.t[5]);
        putLinePack(x+2, pack.t[0], pack.t[1], pack.t[2]);
        break;
      case 2:
        putLinePack(  x, pack.t[8], pack.t[5], pack.t[2]);
        putLinePack(x+1, pack.t[7], pack.t[4], pack.t[1]);
        putLinePack(x+2, pack.t[6], pack.t[3], pack.t[0]);
        break;
      case 3:
        putLinePack(  x, pack.t[2], pack.t[1], pack.t[0]);
        putLinePack(x+1, pack.t[5], pack.t[4], pack.t[3]);
        putLinePack(x+2, pack.t[8], pack.t[7], pack.t[6]);
        break;
      default:
        assert(false);
    }
  }

  /**
   * 指定したx座標にパックの一部を落とす (下のような感じで落とす)
   *
   *    t0
   *    t1
   *    t2
   *
   * @param [int] x x座標
   * @param [char] t0 パックのブロックの値
   * @param [char] t1 パックのブロックの値
   * @param [char] t2 パックのブロックの値
   */
  void putLinePack(int x, char t0, char t1, char t2) {
    int y = g_myPutPackLine[x];

    if (t2 != EMPTY) { g_myField[x][y] = t2; y++; }
    if (t1 != EMPTY) { g_myField[x][y] = t1; y++; }
    if (t0 != EMPTY) { g_myField[x][y] = t0; y++; }

    g_myPutPackLine[x] = y;
  }

  /**
   * 連鎖判定を行う
   *
   * @return [int] 連鎖カウント
   */
  int chainPack() {
    memset(g_packDeleteCount, 0, sizeof(g_packDeleteCount));

    for (int y = 0; y < HEIGHT; y++) {
      deleteCheckHorizontal(y);
    }
    for (int x = 0; x < WIDTH; x++) {
      deleteCheckVertical(x);
    }

    int deleteCnt = calcDeletePack();
    return deleteCnt;
  }

  /**
   * 連鎖判定で削除となったパックを消す
   *
   * @return [int] 削除カウント
   */
  int calcDeletePack() {
    int deleteCnt = 0;

    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        deleteCnt += g_packDeleteCount[y][x];
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
    int from = 0;
    int to = 0;
    char sum = g_myField[to][y];

    while (to < WIDTH) {
      if (sum < DELETED_SUM) {
        if (to < WIDTH-1) break;

        to++;
        if (sum == 0) from = to;

        sum += g_myField[to][y];
      } else {
        sum -= g_myField[from][y];
        from++;
      }

      if (sum == DELETED_SUM) {
        for (int x = from; x <= to; x++) {
          g_packDeleteCount[y][x]++;
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
    int from = 0;
    int to = 0;
    char sum = g_myField[x][to];

    while (to < HEIGHT) {
      if (sum < DELETED_SUM) {
        if (to < HEIGHT-1) break;

        to++;
        if (sum == 0) from = to;

        sum += g_myField[x][to];
      } else {
        sum -= g_myField[x][from];
        from++;
      }

      if (sum == DELETED_SUM) {
        for (int y = from; y <= to; y++) {
          g_packDeleteCount[y][x]++;
        }
      }
    }
  }

  /**
   * 斜めのラインの削除判定を行う (右上に進む)
   *
   * @param [int] y チェックするy座標
   */
  void deleteCheckDiagonalRightUp(int y) {
    int fromY = y;
    int fromX = 0;
    int toY = y;
    int toX = 0;
    char sum = g_myField[toX][toY];

    while (toX < WIDTH) {
      if (sum < DELETED_SUM) {
        toY--;
        toX++;

        if (sum == 0) {
          fromY = toY;
          fromX = toX;
        }

        sum += g_myField[toX][toY];
      } else {
        sum -= g_myField[fromX][fromY];
        fromY--;
        fromX++;
      }
    }

    if (sum == DELETED_SUM) {
      int i = 0;
      for (int x = fromX; x <= toX; x++) {
        g_packDeleteCount[fromY-i][x]++;
        i++;
      }
    }
  }

  /**
   * フィールド情報を保存する
   */
  void saveField() {
    memcpy(g_tempField, g_myField, sizeof(g_myField));
    memcpy(g_tempPutPackLine, g_myPutPackLine, sizeof(g_myPutPackLine));
  }

  /**
   * フィールド情報を元に戻す
   */
  void rollbackField() {
    memcpy(g_myField, g_tempField, sizeof(g_tempField));
    memcpy(g_myPutPackLine, g_tempPutPackLine, sizeof(g_tempPutPackLine));
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

    // [自分のお邪魔ストック]
    int myOjamaStock;
    cin >> myOjamaStock;

    // [前のターン終了時の自分のフィールド情報]
    int t;
    for (int y = 0; y < FIELD_HEIGHT; y++) {
      for (int x = 0; x < FIELD_WIDTH; x++) {
        cin >> t;
        g_myField[x][FIELD_HEIGHT-y-1] = (char)t;
      }
    }

    cin >> _end_;

    // [相手のお邪魔ストック]
    int enemyOjamaStock;
    cin >> enemyOjamaStock;

    // [前のターン終了時の相手のフィールド情報]
    for (int y = 0; y < FIELD_HEIGHT; y++) {
      for (int x = 0; x < FIELD_WIDTH; x++) {
        cin >> t;
        g_enemyField[x][FIELD_HEIGHT-y-1] = (char)t;
      }
    }

    cin >> _end_;
  }

  /**
   * ブロックを落下させる時に落下させる位置を更新する
   */
  void updatePutPackLine() {
    for (int x = 0; x < WIDTH; x++) {
      setPutPackLine(x);
    }
  }

  /**
   * @param [int] x x座標の値
   */
  void setPutPackLine(int x) {
    int y = 1;

    while (g_myField[x][y] != EMPTY && y < DANGER_LINE) {
      y++;
    }

    g_myPutPackLine[x] = y;
  }
};

int main() {
  Codevs cv;

  cout << "siman" << endl;

  cv.init();

  for (int i = 0; i < MAX_TURN; i++) {
    fprintf(stderr,"turn %d =>\n", i);
    cv.run(i);
  }

  return 0;
}
