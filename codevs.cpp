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
const int WIDTH = 1 + FIELD_WIDTH + 1; // 余分な領域も含めたフィールドの横幅
const int HEIGHT = 1 + FIELD_HEIGHT + 3 + 1; // 余分な領域も含めたフィールドの縦幅
const int DANGER_LINE = FIELD_HEIGHT + 1; // 危険ライン

const char DELETED_SUM = 10; // 消滅のために作るべき和の値

const char EMPTY = 0; // 空のグリッド
const char OJAMA = 11; // お邪魔ブロック

int BASE_BEAM_WIDTH = 800; // ビーム幅
int BEAM_WIDTH;
int SEARCH_DEPTH = 18; // ビームサーチの探索の深さ
int g_scoreLimit = 600; // 発火する閾値のスコア

/**
 * 乱数生成器
 */
unsigned long long xor128(){
  static unsigned long long rx=123456789, ry=362436069, rz=521288629, rw=88675123;
  unsigned long long rt = (rx ^ (rx<<11));
  rx=ry; ry=rz; rz=rw;
  return (rw=(rw^(rw>>19))^(rt^(rt>>8)));
}

/**
 * パックの構造体
 */
struct Pack {
  int t[9];

  Pack () {
    memset(t, -1, sizeof(t));
  }
};

char g_myField[WIDTH][HEIGHT]; // 自フィールド
char g_enemyField[WIDTH][HEIGHT]; // 敵フィールド
char g_field[WIDTH][HEIGHT]; // フィールド
char g_tempField[WIDTH][HEIGHT];

int g_putPackLine[WIDTH]; // 次にブロックを設置する高さを保持する配列
int g_tempPutPackLine[WIDTH]; // 一時保存用
int g_maxHeight; // フィールドの最大の高さ

ll g_packDeleteChecker[WIDTH][HEIGHT]; // 連鎖で消えるブロックをチェックするやつ
ll g_zoblishField[WIDTH][HEIGHT][12]; // zoblish hash生成用の乱数テーブル

ll g_chainCheckHorizontal[HEIGHT]; // 連鎖判定フィールド
ll g_chainCheckVertical[WIDTH]; // 連鎖判定フィールド
ll g_chainCheckRightUpH[WIDTH]; // 連鎖判定フィールド
ll g_chainCheckRightUpV[HEIGHT]; // 連鎖判定フィールド
ll g_chainCheckLeftUpV[HEIGHT]; // 連鎖判定フィールド
ll g_chainCheckLeftUpH[WIDTH]; // 連鎖判定フィールド
ll g_checkId;
ll g_deleteId;

int g_deleteCount;

Pack g_packs[MAX_TURN]; // パック一覧
Pack g_ojamaPacks[MAX_TURN]; // お邪魔で埋め尽くされたパック一覧

/**
 * シミュレータに返すコマンドの構造体
 */
struct Command {
  int pos;
  int rot;

  Command (int pos = 0, int rot = 0) {
    this->pos = pos;
    this->rot = rot;
  }
};

struct Action {
  Command command;
  int score;
  int fireTurn;

  Action(Command command = Command(), int score = 0, int fireTurn = MAX_TURN) {
    this->command = command;
    this->score = score;
    this->fireTurn = fireTurn;
  }
};

struct Node {
  int value;
  int score;
  bool chain;
  Command command;
  char field[WIDTH][HEIGHT];

  Node () {
    this->value = 0;
    this->chain = false;
  }

  ll hashCode() {
    ll hash = 0;

    for (int x = 1; x <= FIELD_WIDTH; ++x) {
      for (int y = 1; y <= FIELD_HEIGHT; ++y) {
        int num = this->field[x][y];
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
  int myRemainTime;
  int beforeTime;
  int myOjamaStock;

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
    memset(g_field, EMPTY, sizeof(g_field));
    memset(g_chainCheckHorizontal, -1, sizeof(g_chainCheckHorizontal));
    memset(g_chainCheckVertical, -1, sizeof(g_chainCheckVertical));
    memset(g_chainCheckRightUpH, -1, sizeof(g_chainCheckRightUpH));
    memset(g_chainCheckRightUpV, -1, sizeof(g_chainCheckRightUpV));
    memset(g_chainCheckLeftUpH, -1, sizeof(g_chainCheckLeftUpH));
    memset(g_chainCheckLeftUpV, -1, sizeof(g_chainCheckLeftUpV));
    memset(g_packDeleteChecker, -1, sizeof(g_packDeleteChecker));
    memset(g_putPackLine, -1, sizeof(g_putPackLine));

    g_putPackLine[0] = HEIGHT;
    g_putPackLine[FIELD_WIDTH+1] = HEIGHT;

    g_checkId = 0;
    g_deleteId = 0;

    for (int x = 0; x < WIDTH; ++x) {
      for (int y = 0; y < HEIGHT; ++y) {
        for (int i = 0; i < 12; i++) {
          g_zoblishField[x][y][i] = xor128();
        }
      }
    }

    beforeTime = 180000;

    readPackInfo();
  }

  /**
   * パック情報を読み込む
   * お邪魔が全ての埋まった状態のパックも用意しておく
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
      g_ojamaPacks[i] = pack;

      for (int j = 0; j < 9; j++) {
        if (g_ojamaPacks[i].t[j] == EMPTY) {
          g_ojamaPacks[i].t[j] = OJAMA;
        }
      }

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

    if (myOjamaStock > 0) {
      fillOjama(turn, myOjamaStock);
    }

    Action action = getMyBestAction(turn);
    Command command = action.command;
    int MNP = action.score;

    fprintf(stderr,"%2d: MNP=%d, FT=%d\n", turn, MNP, action.fireTurn);

    cout << command.pos-1 << " " << command.rot << endl;
    fflush(stderr);

    if (myOjamaStock > 0) {
      cleanOjama(turn, myOjamaStock);
    }
  }

  /**
   * 自分のベストなコマンドを選択する
   *
   * @return [Action] 一番良いアクション
   */
  Action getMyBestAction(int turn) {
    memcpy(g_field, g_myField, sizeof(g_myField));

    if (myRemainTime >= 60000) {
      BEAM_WIDTH = 3 * BASE_BEAM_WIDTH;
    } else if (myRemainTime < 30000) {
      BEAM_WIDTH = BASE_BEAM_WIDTH / 2;
    } else {
      BEAM_WIDTH = BASE_BEAM_WIDTH;
    }

    return getBestAction(turn);
  }

  /**
   * 一番良い操作を取得する
   *
   * @param [int] turn 今現在のターン
   * @return [Action] 一番ベストな行動情報
   */
   Action getBestAction(int turn) {
    Node root;
    memcpy(root.field, g_field, sizeof(g_field));
    Action bestAction;
    int maxValue = -9999;

    queue<Node> que;
    que.push(root);

    unordered_map<ll, bool> checkNodeList;

    for (int depth = 0; depth < SEARCH_DEPTH; depth++) {
      priority_queue<Node, vector<Node>, greater<Node> > pque;
      Pack pack = g_packs[turn+depth];
      bool update = false;

      while (!que.empty()) {
        Node node = que.front(); que.pop();
        memcpy(g_field, node.field, sizeof(node.field));
        updatePutPackLine();
        memcpy(g_tempPutPackLine, g_putPackLine, sizeof(g_putPackLine));

        for (int x = 1; x <= FIELD_WIDTH-2; ++x) {
          for (int rot = 0; rot < 4; rot++) {

            if (putPack(x, rot, pack)) {
              Node cand;
              cand.score = simulate();

              if (g_maxHeight < DANGER_LINE) {
                cand.value = cand.score;
                memcpy(cand.field, g_field, sizeof(g_field));
                cand.chain = (cand.score > 1 || (depth > 0 && cand.score > 0));
                if (!cand.chain) {
                  cand.value += evaluate();
                  update = true;
                }
                if (cand.score >= g_scoreLimit) cand.value += 100 * cand.score;
                cand.command = (depth == 0)? Command(x, rot) : node.command;
                pque.push(cand);
              }
            }

            memcpy(g_field, node.field, sizeof(node.field));
            memcpy(g_putPackLine, g_tempPutPackLine, sizeof(g_tempPutPackLine));
          }
        }
      }

      for (int j = 0; j < BEAM_WIDTH && !pque.empty(); j++) {
        Node node = pque.top(); pque.pop();

        if (node.score >= g_scoreLimit) {
          return Action(node.command, node.score, turn+depth);
        }

        if (maxValue < node.score) {
          maxValue = node.score;
          bestAction = Action(node.command, node.score, turn+depth);
        }

        if (depth < SEARCH_DEPTH-1) {
          ll hash = node.hashCode();

          if (!checkNodeList[hash] && (!node.chain || (!update && node.score <= 2))) {
            checkNodeList[hash] = true;
            que.push(node);
          } else {
            j--;
          }
        }
      }
    }

    return bestAction;
  }

  /**
   * パックの落下処理と削除処理を行う
   */
  void fallPack() {
    ++g_checkId;

    for (int x = 1; x <= FIELD_WIDTH; ++x) {
      int fallCnt = 0;
      int limitY = g_putPackLine[x];

      for (int y = 1; y < limitY; ++y) {
        if (g_packDeleteChecker[x][y] == g_deleteId) {
          g_field[x][y] = EMPTY;
          fallCnt++;
          g_putPackLine[x]--;
        } else if (fallCnt > 0) {
          int t = y-fallCnt;
          g_field[x][t] = g_field[x][y];
          g_field[x][y] = EMPTY;
          setChainCheckId(t, x);
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
   * @return [bool] 設置が成功したかどうか
   */
  bool putPack(int x, int rot, const Pack &pack) {
    bool success = true;
    g_checkId++;

    switch (rot) {
      case 0:
        success &= putLinePack(  x, pack.t[6], pack.t[3], pack.t[0]);
        success &= putLinePack(x+1, pack.t[7], pack.t[4], pack.t[1]);
        success &= putLinePack(x+2, pack.t[8], pack.t[5], pack.t[2]);
        break;
      case 1:
        success &= putLinePack(  x, pack.t[8], pack.t[7], pack.t[6]);
        success &= putLinePack(x+1, pack.t[5], pack.t[4], pack.t[3]);
        success &= putLinePack(x+2, pack.t[2], pack.t[1], pack.t[0]);
        break;
      case 2:
        success &= putLinePack(  x, pack.t[2], pack.t[5], pack.t[8]);
        success &= putLinePack(x+1, pack.t[1], pack.t[4], pack.t[7]);
        success &= putLinePack(x+2, pack.t[0], pack.t[3], pack.t[6]);
        break;
      case 3:
        success &= putLinePack(  x, pack.t[0], pack.t[1], pack.t[2]);
        success &= putLinePack(x+1, pack.t[3], pack.t[4], pack.t[5]);
        success &= putLinePack(x+2, pack.t[6], pack.t[7], pack.t[8]);
        break;
      default:
        assert(false);
    }

    return success;
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
   * @param [bool] 設置可能かどうか
   */
   bool putLinePack(int x, int t0, int t1, int t2) {
    int y = g_putPackLine[x];

    assert(t0 >= 0);
    assert(t1 >= 0);
    assert(t2 >= 0);

    if (t0 != EMPTY) {
      if (x < 1 || x > FIELD_WIDTH) return false;
      g_field[x][y] = t0;
      setChainCheckId(y, x);
      ++y;
    }
    if (t1 != EMPTY) {
      if (x < 1 || x > FIELD_WIDTH) return false;
      g_field[x][y] = t1;
      setChainCheckId(y, x);
      ++y;
    }
    if (t2 != EMPTY) {
      if (x < 1 || x > FIELD_WIDTH) return false;
      g_field[x][y] = t2;
      setChainCheckId(y, x);
      ++y;
    }

    assert(y <= HEIGHT);
    g_putPackLine[x] = y;
    return true;
  }

  /**
   * 連鎖判定が必要な場所にチェックを行う
   *
   * @param [int] y ブロックが落下したy座標
   * @param [int] x ブロックが落下したx座標
   */
  void setChainCheckId(int y, int x) {
    g_chainCheckHorizontal[y] = g_checkId;
    g_chainCheckVertical[x] = g_checkId;
    if (x-y >= 0) g_chainCheckRightUpH[x-y+1] = g_checkId;
    if (x-y < 0) g_chainCheckRightUpV[y-x+1] = g_checkId;
    if (y-(FIELD_WIDTH-x) >= 2) g_chainCheckLeftUpV[y-(FIELD_WIDTH-x)] = g_checkId;
    if (x+(y-1) <= FIELD_WIDTH) g_chainCheckLeftUpH[x+(y-1)] = g_checkId;
  }

  /**
   * パックにお邪魔を埋め込む
   *
   * @param [int] turn 現在のターン
   * @param [int] ojamaStock 現在のお邪魔のストック数
   */
  void fillOjama(int turn, int ojamaStock) {
    for (int t = turn; t < MAX_TURN && ojamaStock > 0; t++) {
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
   *
   * @param [int] turn 現在のターン
   * @param [int] ojamaStock 現在のお邪魔のストック数
   */
  void cleanOjama(int turn, int ojamaStock) {
    for (int t = turn; t < MAX_TURN && ojamaStock > 0; t++) {
      Pack *pack = &g_packs[t];

      for (int i = 0; i < 9 && ojamaStock > 0; i++) {
        if (pack->t[i] == OJAMA) {
          pack->t[i] = EMPTY;
          ojamaStock--;
        }
      }
    }
  }

  int evaluate() {
    int maxValue = 0;
    memcpy(g_tempField, g_field, sizeof(g_field));

    for (int x = 2; x <= 2; x++) {
      int y = g_putPackLine[x];

      for (char num = 1; num <= 9; num++) {
        g_checkId++;
        g_field[x][y] = num;
        setChainCheckId(y, x);
        g_putPackLine[x]= y+1;

        int score = simulate();
        maxValue = max(maxValue, score);

        memcpy(g_field, g_tempField, sizeof(g_tempField));
        g_putPackLine[x] = y;
      }
    }

    return maxValue;
  }

  /**
   * 連鎖処理のシミュレーションを行う
   *
   * @return [int] スコア
   */
  int simulate() {
    int chainCnt = 0;
    int score = 0;

    while (true) {
      updateMaxHeight();
      chainPack();
      fallPack();

      if (g_deleteCount == 0) break;

      chainCnt++;
      score += floor(pow(1.3, chainCnt)) * (g_deleteCount/2);
    }

    return score;
  }

  /**
   * 連鎖判定を行う
   */
  void chainPack() {
    g_deleteCount = 0;
    g_deleteId++;

    for (int y = 1; y <= g_maxHeight; ++y) {
      if (g_chainCheckHorizontal[y] == g_checkId) deleteCheckHorizontal(y);
      if (g_chainCheckRightUpV[y] == g_checkId) deleteCheckDiagonalRightUp(y, 1);
      if (g_chainCheckLeftUpV[y] == g_checkId) deleteCheckDiagonalLeftUp(y, FIELD_WIDTH);
    }
    for (int x = 1; x <= FIELD_WIDTH; ++x) {
      if (g_chainCheckVertical[x] == g_checkId) deleteCheckVertical(x);
      if (g_chainCheckRightUpH[x] == g_checkId) deleteCheckDiagonalRightUp(1, x);
      if (g_chainCheckLeftUpH[x] == g_checkId) deleteCheckDiagonalLeftUp(1, x);
    }
  }

  /**
   * 横のラインの削除判定を行う
   *
   * @param [int] y チェックするy座標
   */
  void deleteCheckHorizontal(int y) {
    int fromX = 1;
    int toX = 1;
    char sum = g_field[toX][y];

    while (toX <= FIELD_WIDTH) {
      if (sum < DELETED_SUM) {
        toX++;

        if (sum == 0) {
          fromX = toX;
        }

        char num = g_field[toX][y];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
        } else {
          sum += num;
        }
      } else {
        assert(g_field[fromX][y] != EMPTY);
        sum -= g_field[fromX][y];
        fromX++;

        if (fromX > toX) {
          toX = fromX;
          sum = g_field[toX][y];
        }
      }

      if (sum == DELETED_SUM) {
        assert(0 <= fromX && toX < WIDTH);
        g_deleteCount += (toX-fromX+1);
        for (int x = fromX; x <= toX; ++x) {
          g_packDeleteChecker[x][y] = g_deleteId;
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
    int fromY = 1;
    int toY = 1;
    char sum = g_field[x][toY];

    while (toY <= g_maxHeight) {
      if (sum < DELETED_SUM) {
        toY++;

        if (sum == 0) {
          fromY = toY;
        }

        char num = g_field[x][toY];
        if (num == EMPTY) break;

        if (num == OJAMA) {
          sum = 0;
        } else {
          sum += num;
        }
      } else {
        sum -= g_field[x][fromY];
        fromY++;

        if (fromY > toY) {
          toY = fromY;
          sum = g_field[x][toY];
        }
      }

      if (sum == DELETED_SUM) {
        g_deleteCount += (toY-fromY+1);
        for (int y = fromY; y <= toY; ++y) {
          g_packDeleteChecker[x][y] = g_deleteId;
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
    char sum = g_field[toX][toY];

    while (toX <= FIELD_WIDTH && toY <= g_maxHeight) {

      if (sum < DELETED_SUM) {
        toY++;
        toX++;

        if (sum == 0) {
          fromY = toY;
          fromX = toX;
        }

        char num = g_field[toX][toY];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
        } else {
          sum += num;
        }
      } else {
        assert(g_field[fromX][fromY] != EMPTY);
        sum -= g_field[fromX][fromY];
        fromY++;
        fromX++;

        if (fromX > toX) {
          toX = fromX;
          toY = fromY;
          sum = g_field[toX][toY];
        }
      }

      if (sum == DELETED_SUM) {
        int i = 0;
        assert(0 <= fromX && toX < WIDTH);
        g_deleteCount += (toX-fromX+1);
        for (int x = fromX; x <= toX; ++x) {
          g_packDeleteChecker[x][fromY+i] = g_deleteId;
          i++;
        }
      }
    }
  }

  /**
   * 連鎖判定 (左上に進む)
   *
   * @param [int] sy チェックするy座標
   * @param [int] sx チェックするx座標
   */
  void deleteCheckDiagonalLeftUp(int sy, int sx) {
    int fromY = sy;
    int fromX = sx;
    int toY = sy;
    int toX = sx;
    char sum = g_field[toX][toY];

    while (toX >= 1 && toY <= g_maxHeight) {
      if (sum < DELETED_SUM) {
        toY++;
        toX--;

        if (sum == 0) {
          fromY = toY;
          fromX = toX;
        }

        int num = g_field[toX][toY];

        if (num == EMPTY || num == OJAMA) {
          sum = 0;
        } else {
          sum += num;
        }
      } else {
        assert(g_field[fromX][fromY] != EMPTY);
        sum -= g_field[fromX][fromY];
        fromY++;
        fromX--;

        if (fromX < toX) {
          toY = fromY;
          toX = fromX;
          sum = g_field[toX][toY];
        }
      }

      if (sum == DELETED_SUM) {
        int i = 0;
        g_deleteCount += (fromX-toX+1);
        for (int x = toX; x <= fromX; ++x) {
          g_packDeleteChecker[x][toY-i] = g_deleteId;
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
    cin >> myRemainTime;

    fprintf(stderr,"%2d:MRT=%d,UT=%d\n", turn, myRemainTime, beforeTime - myRemainTime);
    beforeTime = myRemainTime;

    // [自分のお邪魔ストック]
    cin >> myOjamaStock;


    // [前のターン終了時の自分のフィールド情報]
    int t;
    for (int y = 0; y < FIELD_HEIGHT; ++y) {
      for (int x = 1; x <= FIELD_WIDTH; ++x) {
        cin >> t;
        g_myField[x][FIELD_HEIGHT-y] = t;
      }
    }

    cin >> _end_;

    // [相手のお邪魔ストック]
    int enemyOjamaStock;
    cin >> enemyOjamaStock;

    int ojamaCnt = 0;

    // [前のターン終了時の相手のフィールド情報]
    for (int y = 0; y < FIELD_HEIGHT; ++y) {
      for (int x = 1; x <= FIELD_WIDTH; ++x) {
        cin >> t;
        g_enemyField[x][FIELD_HEIGHT-y] = t;
        if (t == OJAMA) {
          ojamaCnt++;
        }
      }
    }

    if (ojamaCnt >= 25) {
      g_scoreLimit = 120;
    }

    cin >> _end_;
  }

  /**
   * ブロックを落下させる時に落下させる位置を更新する
   */
  inline void updatePutPackLine() {
    for (int x = 1; x <= FIELD_WIDTH; ++x) {
      setPutPackLine(x);
    }
  }

  /**
   * @param [int] x x座標の値
   */
  inline void setPutPackLine(int x) {
    int y = 1;

    while (g_field[x][y] != EMPTY && y < HEIGHT-1) {
      ++y;
    }

    g_putPackLine[x] = y;
  }

  /**
   * フィールドの最大の高さを更新する
   */
  inline void updateMaxHeight() {
    g_maxHeight = 0;

    for (int x = 1; x <= FIELD_WIDTH; ++x) {
      g_maxHeight = max(g_maxHeight, g_putPackLine[x]-1);
    }
  }

  /**
   * フィールドを表示する(デバッグ用)
   */
  void showField() {
    fprintf(stderr,"\n");
    for (int x = 0; x < WIDTH; ++x) {
      for (int y = 0; y < HEIGHT; ++y) {
        if (g_field[x][y] == 11) {
          fprintf(stderr,"B");
        } else {
          fprintf(stderr,"%d", g_field[x][y]);
        }
      }
      fprintf(stderr," %d\n", g_putPackLine[x]);
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
