# codevs_student
CODE VS for STUDENT

## 実装するものリスト

* [x] ゲーム開始時のデータの読み込み
  * [x] ゲーム情報の読み込み
  * [x] パック情報の読み込み
* [x] ターン毎の入力の読み込み
  * [x] 現在のターン数
  * [x] 自分の残り思考時間
  * [x] 自分のお邪魔ストック
  * [x] 前のターン終了時の自分のフィールド情報
  * [x] 相手のお邪魔ストック
  * [x] 前のターン終了時の相手のフィールド情報
* [x] 自分のフィールドの各高さを求める
  * [x] 最も高いブロックの位置と最も低いブロックの位置を求める (必要になったら)
* [x] ブロックの設置処理 (最初にブロックを置く処理)
* [x] ブロックの落下処理 (ブロック消滅後の処理)
* [x] 落とせる全ての方法を試す
  * [x] [-2..9] * 4 通りすべて落とす
    * [x] 落とす前にフィールド情報を保存しておく
    * [x] 落とす
    * [x] 落とす度にフィールド情報をリセットする
* [x] 連鎖処理
  * [x] ブロックの消滅判定 (出来れば高速に判定したい)
    * [x] 横の消滅判定
    * [x] 縦の消滅判定
    * [x] 左端から右上に進む消滅判定
    * [x] 左端から右下に進む消滅判定
    * [x] 消滅カウントの計算
  * [x] 消滅した場合はブロックを落下。判定を続行
* [x] zoblishハッシュの生成
  * [x] 初期化時に乱数テーブルを生成
* [x] ノードの構造体
  * [x] フィールド情報
  * [x] 評価値
* [x] 現在の状態を保持したノードを作成する
  * [x] フィールドのコピー
  * [x] 評価値の計算
* [x] お邪魔ブロックの処理
  * [x] パック情報にお邪魔を追加していく
  * [x] スタックがある時に追加する
* [x] フィールド情報を保存する
* [x] フィールドを保存した情報に戻す
* [x] フィールドが有効な状態かどうかをチェックする
- [ ] フィールドchar化大作戦


## 仕様変更 (優先度高い)

* [ ] フィールドの縦横を入れ替える


## 考察

まぁシミュレータ部分を高速化して読み手を深くして殴るのが一番正攻法な気がする

* 1手最大 4 * 12 = 48 通りの手が存在している
* 最初のターンで全てのパックの情報が取得出来るっぽい？
* 何手先まで読むのが良いか
* 相手の動向をどう読むか
* 制限時間は3分 (180秒) 1手あたり300msec使用できる
  * 最初は自由に組めるので時間を長く使ってもいいかもしれない
  * 相手の様子次第で読みの深さと幅を動的に変更する
  * 相手の読みを入れると200msec程度になりそう？

* フィールド情報は90度回転させた状態で保持したほうがいい気がする (そっちのほうがまだ直感的)
* フィールドの大きさは 20*19 = 380 byte （計算は全てcharで行うこと）

### 連鎖と火力について

連鎖と火力についての関係を調査して、どれが一番効率が良いかを調べる

## ゲームの流れ

* パックの投下
* ブロックの消滅判定
  * ブロックが消滅した場合は再度ブロックを落として消滅判定

## ブロックの消滅

* 連続したブロックの数値の合計がある一定の値になっている
* 消滅カウントはその数値の和を満たしたブロックの総計値（重複する場合もある）

## ゲームの終了条件

* デンジャーラインまでブロックが積み上がる
* 思考時間が制限を超える
* ターン数が一定数を超える
* 不正な入力

## 思考時間の制限

## ターンについて

* 1ゲーム最大500ターン

## パック

* 3x3の大きさ

## ブロックの数値

* ブロックの数値は[1-9]
* 全てのターンを通して[1-9]のブロックはそれぞれ180個与えられる

## 消滅条件の和

* 総和は10で消える

## お邪魔ブロック

* ブロックの数値が11の値なブロックが振ってくる
* お邪魔ストックが存在する場合に、NEXTパックにお邪魔パックが設定される
* お邪魔ブロックは左上から右下へ順番に挿入される

## パックの投下位置

* [-2..9]が指定出来る

## 思考時間

* 1試合5分
