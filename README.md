# medama

M5Stack StopWatch C152 の丸い画面に、目をひとつ表示する firmware です。操作なしで視線が移動・静止し、ときどき閉眼して再び開きます。同じ動きを、赤い虹彩・小さな瞳孔・右上のピンクのハイライトを持つ漫画風（目玉親父風）と、固定の虹彩模様・柔らかな陰影・光の映り込みを持つリアル風で描き分けます。

黄色の **BtnA を短押しして離すたび**に、漫画風とリアル風を交互に切り替えます。保持中の連続切り替えはなく、長押しでは切り替えません。切り替えても視線・まばたきの進行は維持します。起動時は毎回漫画風で、選択は保存しません。BtnB と電源ボタンにはアプリ操作を追加していません。

[Issue #2](https://github.com/minorunakamura/medama/issues/2) の最新ソースは host test と firmware compile／link で確認したもので、**実機未検証**です。表示品質・実ボタン応答・FPS・長時間稼働・電池持ちは保証しません。今回 USB／serial open・実機情報取得・reset・Flash 操作・upload・monitor は行っていません。過去の Issue #1 では別途ユーザー許可のもと、漫画風描画と電源再投入時の黒画面修正を実機確認しましたが、その履歴を今回の版の実機検証としては扱いません。

GitHub Actions は `main` への push と `main` 向け Pull Request で、host test と firmware build を実行します。実機への書き込みは行いません。

環境準備、固定依存、操作、test／build、CI、成果物、cleanup と未確認事項は [開発手順](docs/development.md) を参照してください。
