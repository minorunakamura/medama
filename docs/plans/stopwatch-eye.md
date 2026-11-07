# StopWatch 目玉表示 — ソース実装・ビルド確認 Plan

## 目的

M5Stack StopWatch C152 の丸い画面にひとつの目を表示し、自律的な視線移動・まばたきと、リアル風／目玉親父風の切り替えを備える firmware を新規に実装する。

今回の成果はソース、限定的な自動テスト、再現可能なビルド手順とビルド成果物までとする。実機への書き込み・表示確認は含めず、実機での完成度を検証済みとは扱わない。

## 入力と確認済みの事実

- 正式な入力設計：`docs/designs/stopwatch-eye.md`。特に「目的と範囲」「表示と振る舞い」「設計の要点」「後続で決めることと未検証事項」を維持する。
- リポジトリには基本設計だけがあり、既存の production code、テスト、ビルド設定、検証 Skill／Feature Map、README はない。既存設計は未追跡ファイルとして存在し、変更・削除しない。既存利用者向け API や移行対象データはない。
- ローカルでは `uv 0.13.5`、Python 3.14.9、Apple clang++ を確認した。PlatformIO／Arduino CLI は PATH 上に見つからなかった。環境準備やビルドは未実施。
- 公式製品仕様は ESP32-S3R8、Flash 16MB、PSRAM 8MB、CO5300／QSPI、円形 AMOLED 466×466。プログラマブルボタンは黄色の KEYA と青色の KEYB、別に電源ボタンを備える。
- 公式製品ページには PlatformIO の `espressif32 @ 6.12.0`、`board = esp32s3box`、Arduino framework、16MB Flash／OPI PSRAM 用設定と M5Unified／M5GFX 等の依存例がある。独自 driver は追加しない。
- M5GFX の board spec には 468×468 の記載もある。製品仕様との差の理由は未解決であり、固定解像度や周辺の見え方を実機確認済みとしない。

## 要件・決定事項

1. 画面の円形領域を主に使うひとつの目を描く。顔全体や歩行キャラクターは描かない。
2. 操作なしで視線が移動し、移動の間に静止し、ときどきまばたきする。人の認識・追跡ではない。
3. 視線とまぶたの開き具合は共通の状態とし、ふたつの描き方で共有する。
4. リアル風は虹彩の模様、柔らかな陰影、光の映り込みで質感を表現し、過度な生々しさを避ける。目玉親父風は白目とシンプルな黒目を中心とする。
5. 本体のプログラマブルボタンの短押しでふたつの見た目を交互に切り替える。具体的な割り当ては黄色の BtnA とし、押し続けている間の連続切り替えは行わない。BtnB にはアプリ操作を追加せず、電源ボタンは流用しない。
6. 起動時は毎回目玉親父風とし、選択状態を保存しない。見た目の切り替えは共通の動きの状態をリセットしない。
7. 速さ、視線範囲、静止時間、まばたき間隔、色や陰影は、目的に沿う初期定数として実装する。設定画面や外部設定は追加しない。実画面での最終調整は今回行わない。
8. 開発環境は C++／Arduino framework／PlatformIO、表示・ボタン操作は M5Unified／M5GFX を採用する。
9. 環境準備は既存の `uv` による `.venv` と PlatformIO の保存先指定で依存を分ける方式とする。後続 Implementation での当該環境への package／toolchain／公式ライブラリ取得はユーザー確認済み。

短押し操作、起動時の見た目、開発環境は基本設計の未決事項を具体化したもの。実機確認を今回の作業範囲から除くことはユーザー確認済みであり、基本設計の見た目・性能の評価を済ませたことにはしない。

## 制約・対象外

- USB／serial の接続操作、ポートの open、実機情報取得、reset、Flash 読出し・消去・書込み、M5Burner、upload、monitor は実行しない。見えているポートを対象機と推測しない。
- 音声、通信、外部サービス・PC 連携、LLM、傾きやタッチへの反応、時刻表示、設定永続化は実装しない。
- browser／desktop preview、simulator、恒久 E2E、CI、新しい test framework、独自 GUI／driver、plugin／テーマ基盤は追加しない。
- 実機 FPS、ティアリング、画面端、色味、実機メモリ確保、実ボタン応答、生き物らしさ、不気味さ、長時間稼働、電池持ちは保証対象外。
- global install、shell 設定変更、既存の共有開発環境変更、新しい Python の自動取得は行わない。互換性不足時に別 toolchain／framework へ無断で切り替えない。
- Planning では production code、環境構築、検証資産の生成を行わない。Implementation は Work Items 確定後の別セッションとする。

## 実装方針

### 1. 再現可能な最小ビルド環境

- `platformio.ini` の firmware 環境を `m5stack-stopwatch` とする。公式 PlatformIO 例を基準に `espressif32 @ 6.12.0`、`esp32s3box`、Arduino、`default_16MB.csv`、16MB Flash、`qio_opi`、PSRAM／USB CDC の必要な build flags を設定する。driver 側の StopWatch 対応・機種選択経路も確認する。
- PlatformIO Core は Python 3.14 対応が公式 release note にある `6.1.19` を初期採用版として固定する。M5Unified／M5GFX と必要な公式依存は Implementation 時に StopWatch 対応と互換条件を確認し、実在する release または commit に固定する。最終設定に可変の master／latest は残さない。取得された framework／toolchain 等の版もビルド記録に残す。
- `uv venv` で、確認した既存 Python を明示して `.venv` を作成する。`uv pip install --python .venv/bin/python` で固定版 Core を導入する。Python の自動ダウンロードを禁止し、global Python は変更しない。
- `PLATFORMIO_CORE_DIR` をプロジェクト内 `.cache/platformio` に指定する。workspace は標準の `.pio` を使い、uv の cache はプロジェクト内に指定するか `--no-cache` を使う。環境変数は実行用の subshell に限定し、既存の上書き設定と保存先を確認する。
- `.venv/`、対象の `.cache/` 配下、`.pio/` 等の生成物を必要最小限の `.gitignore` で除外する。既存ディレクトリがあれば所有関係を確認し、上書き・一括削除しない。
- この方法は依存と保存先の分離であり、security sandbox／container ではない。OS の一時ファイルや外部へのアクセスを全面的に封じる保証はない。
- firmware の build／clean のみ実行する。互換エラーでは原因と試した方法を記録する。別 Python の導入、共有環境変更、採用 framework の変更が必要なら許可・再計画の経路に戻り、ビルド成功を偽らない。

### 2. アプリと状態更新

- Arduino の `setup`／`loop` を入口にし、M5Unified の公式初期化・ボタン API を使う。不要な周辺機器の初期化は可能な範囲で無効にし、Wi-Fi 等のアプリ機能は開始しない。
- 小さなハードウェア非依存の状態更新部を設け、視線位置、移動と静止の進行、まばたきの進行を保持する。描画部とは分離するが、汎用 engine、時計 interface、test 専用 API、大きな fake は作らない。
- 呼び出し元が現在時刻を渡し、乱数 seed を指定できる程度の通常の設計で決定的なテストを可能にする。実機側では起動時に seed を与える。目標位置・間隔を毎フレーム引き直して震えるような動きにしない。
- 視線は移動補間と静止を交互に進め、黒目／虹彩が眼球の不自然な外側へ逸脱しない範囲に制限する。まばたきは開眼→閉眼→開眼へ進める。時刻差は `millis()` の wraparound を考慮し、遅いフレームで無制限の追いつきループを回さない。
- `loop` でボタン更新と状態更新を繰り返し、まばたきや視線待ちのための長い `delay` は使わない。短押しは BtnA の click 相当のイベントを固定した版の公開 API で確認して使い、一操作一切り替えとする。

### 3. 描画

- 小さな描画部が同じ状態を受け取り、選択中の見た目を描く。style 列挙と単純な分岐で足りる構成とし、見た目ごとにアニメーションを複製しない。
- `M5.Display.width()/height()` から中心・半径を求める。466／468 を drawing geometry の定数として埋め込まない。円形画面の前提で配置し、まぶたの mask で閉眼を表す。
- PSRAM の RGB565 `M5Canvas` 一枚を初期化時に確保し、フレームを合成後 `pushSprite()` で転送する。毎フレーム確保し直さない。確保失敗は戻り値で検出し、未確保 buffer を使用せず、最小のエラー表示／診断に留める。
- 単純な円・線・濃淡でふたつの見た目を描く。虹彩の模様は描画ごとにランダム化してちらつかせず、追加の画像素材や filesystem を前提にしない。
- フレーム更新に上限を設けるが、FPS を保証しない。DMA、二重 buffer、複雑な部分転送は初期実装に加えない。Canvas は途中の描画を隠す手段であり、転送のティアリング防止を保証しない。

### 4. テストと文書

- 状態更新の小さな assert ベース C++ test を追加し、既存の clang++ で実行する。追加の test framework や PlatformIO native platform は不要とする。
- README は概要、BtnA 操作、初期 style、未実機検証の明示と詳細文書への入口に留める。
- `docs/development.md` に uv 環境準備、固定版、正確な build／host test コマンド、生成物、cleanup、USB 操作禁止、未確認事項を記載する。
- `docs/designs/stopwatch-eye.md` は今回編集しない。基本設計の未決事項に対する今回の決定は本 Plan を参照し、実装詳細は開発文書から辿れるようにする。GLOSSARY／ADR／CONTRIBUTING を一律に増やさない。

## 完了条件

1. 公式対応ライブラリを用いた StopWatch 向けソースと固定版のビルド設定が揃い、文書の手順で firmware の compile／link が成功する。
2. firmware の `.elf`／`.bin` 等、実際に出力される成果物の所在と build 成功を確認し、使用版を記録できる。
3. 視線移動と静止、まばたき、ふたつの描き方、BtnA による一操作一切り替え、目玉親父風での起動、非永続化が実装されていることをソース review と対象 test で確認する。
4. 限定した host test が成功し、状態更新の範囲・完了・時刻 wraparound の回帰リスクを確認する。
5. README と開発文書から別 Agent が会話履歴なしで環境準備・test・build・成果物確認・所有対象だけの cleanup を再実行できる。
6. 実機未確認の項目を明記し、upload、serial open、実機 reset、Flash 操作を一切行っていない。

これらはソース実装・ビルド範囲の完了条件であり、「実機で自然に見える」「滑らかに動作する」「実ボタンで切り替わる」の受入れを満たしたとは主張しない。

## 動作確認方法

### 保証対象と自動テスト

- 自動テスト対象はハードウェア非依存の状態更新と style 切り替えの小さなロジックに限定する。
- 固定 seed／時刻を用い、視線が所定範囲内に留まること、移動の間に静止すること、まばたきが閉じて再び開くこと、style が交互に切り替わること、style 切り替えが動きの状態をリセットしないことを代表ケースで確認する。
- 実時間の長い待機はせず時刻を進め、32bit 時刻の wraparound と大きめの更新間隔の代表境界も確認する。全組合せや乱数分布の統計 test は追加しない。
- TDD は今回の全対象で必須採用しない。新規の小さな状態更新には実装と近接して決定的な回帰 test を追加し、test-first を選ばないことを test 省略の理由にしない。hardware API／描画品質を host test が保証したことにはしない。

### 直接確認と正式チェック

1. Worker は現在の source／差分と環境を確認し、承認済みの uv 分離環境を準備する。ユーザーの既存設定や未知のディレクトリは変更しない。
2. Worker は文書化した host test の compile／実行と `PLATFORMIO_CORE_DIR="$PWD/.cache/platformio" .venv/bin/pio run -e m5stack-stopwatch` 相当の firmware build を実行する。設定による自動 upload 等がないことを先に確認する。
3. Worker は対象環境、依存版、終了 code、test 結果、firmware 出力ファイルとサイズを確認する。通常の build 警告も確認し、失敗を実機未確認の扱いに紛れ込ませない。
4. Root は最終差分で共通状態の再利用、公開 API の使い方、BtnA のイベント処理、動的画面寸法、未確保 buffer 対策、不要な通信・永続化・実機操作の不在を直接確認する。
5. Root は最終版に対して同じ host test と firmware build／成果物確認を直接再実行し、変更後の結果を採用する。小さな test 成功だけで描画・実機統合まで受け入れない。

### 検証資産・証拠・cleanup

- 既存の検証 Skill／Feature Map／harness はない。今回はユーザー判断で実機 runtime を明示的に対象外とし、代替 preview も作らないため、実アプリを操作する Create の初期対象はない。通常の host test と正式 build 手順を `docs/development.md` に残し、これらだけのために検証 Skill を重複作成しない。
- 現時点では `create-verification-skill` による作成予定なし。ただし、Implementation Root は現在の手段を再確認し、承認済み保証対象・対象外・期待結果を変えずに手段を具体化し、必要なら最小の Create／関連更新、接続・操作・観測用の小さい helper の作成・保存・再利用を判断できる。単に未予定という理由で再計画は要求しない。
- ただし今回の USB／実機操作禁止を読み替えない。Create の成立を示すために書き込む、preview を新設する、未確認の期待結果を緩めることは認めない。公開仕様・設計・正式 Plan の制約や保証を変える場合は確認／REPLAN_REQUIRED に戻る。
- Worker／Root は自身の実行ごとに、source の識別情報と dirty 状態、コマンド、使用版、期待結果と観測結果、成否・未確認範囲を短く記録する。必要最小限の build／test 証拠は `.pi/verification-evidence/<run>/` に置き、当該ディレクトリを Git ignore とする。秘密情報や不要な raw log を保存せず、外部送信しない。
- cleanup は自分が作成した test binary、一時状態、起動した process だけに限定する。再利用可能な `.venv`／PlatformIO cache は開発環境として保持し、削除手順と所有範囲を文書化する。証拠は cleanup で消さず、参照が読めることを確認する。
- 必須権限・依存・安全な実行手段が実際に不足し、許可範囲で成立しなければ BLOCKED とし、試した方法・失敗・未確認の方法、不足と再開条件を区別して報告する。無断 install、独自の必須コンポーネント代替、認証や安全条件の迂回はしない。

## 根拠と未検証事項

- 入力設計：`docs/designs/stopwatch-eye.md`。
- [StopWatch 公式仕様・PlatformIO 設定](https://docs.m5stack.com/ja/core/StopWatch)：仕様、KEY ピンマップ、ソフトウェア／PlatformIO 節。
- [M5GFX StopWatch board spec](https://github.com/m5stack/M5GFX/blob/28d47949bdc84f30881623476f555398e2194313/extras/board_spec/boards/m5stopwatch.json)：468×468 と PSRAM 等の記述。製品仕様との差は未解決。
- [M5Canvas 公式資料](https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_canvas)：offscreen 描画、`createSprite`／`pushSprite`／色深度。
- [PlatformIO Core 6.1.19](https://github.com/platformio/platformio-core/releases/tag/v6.1.19)：Python 3.14 対応追加。ただし、ESP32 platform／付属ツールを含む今回の組合せは未ビルド。
- [PlatformIO core_dir](https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/core_dir.html)：既定 `~/.platformio` と保存対象、`PLATFORMIO_CORE_DIR`。
- uv のローカル公開 help で `venv`、`pip install --python`、`--no-python-downloads`、cache 指定を確認した。これらの確認は環境構築・ビルド成功の証拠ではない。

依存版の実際の互換性、実機機種認識、画面境界、PSRAM 確保、転送性能と見た目の評価は未確認。前者は Implementation の build で確認し、実機依存の項目は今回未確認のまま明示する。