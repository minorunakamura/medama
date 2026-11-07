# medama

M5Stack StopWatch C152 の丸い画面に、目玉親父風の目をひとつ表示する firmware です。赤い虹彩、小さな黒い瞳孔、右上の薄いピンクのハイライトと下側の陰影を描き、操作なしで視線が移動・静止し、ときどき閉眼して再び開きます。

これは [Issue #1](https://github.com/minorunakamura/medama/issues/1) の中間成果です。起動時から漫画風のみで、リアル風・BtnA による見た目の切り替えは未実装（後続 Issue #2）。ボタン操作や設定保存はありません。基本設計／Plan 全体の完成ではありません。

host の状態更新 test と firmware の compile／link に加え、ユーザー許可のある実機調査で電源再投入時の黒画面を修正しました。起動修正は USB 接続で2回と電池駆動で、最新の描画版も電源を入れ直して表示できることをユーザーが確認済みです。FPS・長時間稼働・電池持ちは未検証です。実機操作は対象機器の確認と明示的な許可のある場合に限ります。

GitHub Actions は `main` への push と `main` 向け Pull Request で、host test と firmware build を実行します。実機への書き込みは行いません。

環境準備、固定依存、test／build、CI、成果物と未確認事項は [開発手順](docs/development.md) を参照してください。
