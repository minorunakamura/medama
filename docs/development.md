# 開発・検証手順（Issue #1・#2）

対象は StopWatch C152 の単眼、自律的な視線移動・静止・まばたき、漫画風／リアル風の描き分けと BtnA 切り替え、および build 環境です。Issue #1 の漫画風描画・電源再投入時の黒画面修正を維持し、Issue #2 で見た目の切り替えを追加しました。[正式 Plan](plans/stopwatch-eye.md) と [基本設計](designs/stopwatch-eye.md) は変更しません。

黄色の BtnA を短押しして離すたびに漫画風（目玉親父風）とリアル風を交互に切り替えます。保持中の反復・長押しによる切り替えはありません。視線・まばたきの進行は維持し、毎回漫画風で起動します。選択は保存せず、BtnB／電源ボタンにはアプリ操作を追加していません。Issue #2 の最新版は source／host test／compile・link の確認のみで、実機表示・実ボタンは未確認です。後述の実機調査は過去の Issue #1 の履歴であり、最新版の検証ではありません。

## 安全と前提

- コマンドは repository root で実行します。macOS arm64、既存の uv、Python 3.14.9、Apple clang++ 21.0.0、Git とネットワークを使用しました。
- 今回（Issue #2）は host test と firmware build のみです。USB／serial open、機器情報取得、実機 reset、Flash 読出し・消去・書込み、upload、monitor は一切実行しません。過去の Issue #1 の黒画面調査では別途ユーザーの明示許可と対象照合のもとで実機を操作しましたが、その許可を今回に流用しません。将来の実機操作も対象と新たな許可の確認が必要です。
- global install、共有環境・shell 設定の変更、新しい Python の download はしません。既存 `.venv`／cache の所有者が不明なら上書きせず確認してください。
- `.venv`、`.cache/platformio`、`.pio` の保存先分離は sandbox ではありません。OS 一時ファイルや通信を全面的に隔離するものではありません。
- 既存の `PLATFORMIO_*`／`UV_*`／`PYTHON*` 環境変数は以下で確認します（値に秘密が含まれる環境では記録・共有しない）。今回の実行開始時には該当設定はありませんでした。未知の上書きがあれば確認してから進めます。

```sh
pwd
env | grep -E '^(PLATFORMIO_|UV_|PYTHON)' || true
ls -ld .venv .cache/platformio .pio 2>/dev/null || true
```

## ローカル環境の準備

既存の `.venv`／`.cache/platformio`／`.pio` が正常なら再作成や追加 install は不要です。Issue #2 はそれらを再利用しました。以下は初期準備が必要な場合の手順で、この checkout で確認済みの実行ファイルを明示します。別の checkout では実在する既存 Python／uv のパスを確認して置き換え、見つからない／非互換の場合は停止してください。activate や永続的な export は不要です。

```sh
(
  set -eu
  UV=/Users/minoru/.local/share/mise/installs/uv/latest/uv-aarch64-apple-darwin/uv
  PYTHON=/Users/minoru/.local/share/mise/installs/python/3.14/bin/python3
  "$UV" --version
  "$PYTHON" --version
  "$UV" --no-cache --no-python-downloads venv --python "$PYTHON" .venv
  "$UV" --no-cache --no-python-downloads pip install --python .venv/bin/python \
    'platformio==6.1.19' 'pip==25.3'
  PLATFORMIO_CORE_DIR="$PWD/.cache/platformio" .venv/bin/pio --version
)
```

Issue #1 の初期準備では uv 0.13.5 を使用。pip は PlatformIO の tool-esptoolpy 導入が `.venv/bin/python -m pip ... -t <local package>/_contrib` を呼ぶため必要です。uv venv は既定で pip を含まず、当時の pip なしの初回試行は失敗しました。上記は補完済みの手順です。pip も uv 経由で `.venv` のみに入れます。Core の Python 3.14 対応は [6.1.19 release](https://github.com/platformio/platformio-core/releases/tag/v6.1.19) を基準にしています。

## 固定版と公式の StopWatch 対応

`platformio.ini` は [公式 StopWatch PlatformIO 例](https://docs.m5stack.com/en/core/StopWatch) を基準に、`espressif32 @ 6.12.0`／`esp32s3box`／Arduino、`default_16MB.csv`、16MB Flash、`qio_opi`、PSRAM と USB CDC／USB mode flags を設定しています。verbose debug の flag と monitor 設定は不要なので含めていません。`board_upload.*` は Flash 容量の metadata であり、upload 指定ではありません。自動 upload の target／hook／extra script はありません。追加の `-DBOARD_ID=30` は固定版 M5Unified が提供する機種固定指定です。`30` が `board_M5StopWatch` であることを `src/main.cpp` の `static_assert` で確認します。

ライブラリは実在する次の commit に固定しています（可変 branch は使いません）。M5GFX を先に解決し、M5Unified の `M5GFX >=0.3.0` による別コピー取得を避けます。

| ライブラリ | 版 | commit |
| --- | --- | --- |
| M5GFX | 0.3.0 | `28d47949bdc84f30881623476f555398e2194313` |
| M5Unified | 0.3.0 | `96576a14247ddc935149145aa6b51943464120f1` |
| M5PM1 | 1.0.7 | `315d2899c863ae34b59197392b9f4c59490751ef` |
| M5IOE1 | 1.0.9 | `8401119bf171cb8f906fbe22e7490989077bbee8` |

M5PM1／M5IOE1 は公式例の依存も固定したものです。standalone ライブラリはアプリから直接呼び出さず、起動前の応答確認も M5Unified 内の公開 `M5PM1_Class`／`M5IOE1_Class` を使います。

ビルド後は `.pio/libdeps/m5stack-stopwatch/` の取得ソースを直接確認できます。

- `M5Unified/src/M5Unified.hpp` の `begin(cfg)` は正の `BOARD_ID` を M5GFX の `detect_config.fixed_board` に渡します。`Display.init()` は自動判定や NVS の保存済み候補に依存せず、StopWatch の標準電源投入・表示初期化経路へ進みます。`esp32s3box` はビルド用 board 定義で、実機の表示 driver の選択とは別です。独自 driver や第三者コードの変更はありません。
- 初期版の自動判定は `M5GFX/src/board_detect/m5/esp32s3.inl` の `pm1_family_detector_t` を使っていました。touch がまだ応答しない場合は GPIO の状態から PaperMono 等の候補へ進む経路もあるため、電源再投入後の黒画面への対策として機種を固定しました。機種固定だけでは実機の黒画面は解消しませんでした。その後、IOE1 の起動待ち不足を実機ログから確認し、下記の応答確認を追加しました。
- `M5GFX/src/board_detect/m5/esp32s3_construct.inl` の `construct_stopwatch` は QSPI と CO5300 panel を構成し、CST820／CST9217 の touch controller に対応します。`M5GFX.cpp` の `board_M5StopWatch` 分岐には公式 driver 内の framebuffer 処理があります。
- `M5Unified/src/M5Unified.inl` には StopWatch の I2C pin map／電源・IO expander の起動処理があります。アプリは起動前の応答確認に M5Unified の公開 I2C／電源／IOE1 API を使い、その後は `M5.config()`／`M5.begin(cfg)`／`M5.update()` を使います。IMU／RTC／mic／speaker／外部 display／外部電源等を設定で無効化します。driver 自身が必要とする内部バス／touch 初期化は残りますが、アプリによる touch 反応や通信開始はありません。
- `M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`／`.inl` の公開 `setColorDepth(16)`、`setPsram(true)`、`createSprite()`、`pushSprite()` を使用します。allocator は PSRAM 失敗時に内部 RAM を試すため、アプリは `esp_ptr_external_ram()` でも確認し、不適合なら解放して buffer を使いません。

ソース確認・compile／link と実機観測は区別します。過去の Issue #1 の実機調査では driver が 468×468 と報告し、PSRAM／Canvas 確保と、当時の修正版の電源再投入後の表示を確認しました（後述）。製品仕様 466×466 と board spec 468×468 の差は未解決のまま、描画は `M5.Display.width()/height()` に従います。

Issue #2 の build／`pio pkg list` でも確認した既存 platform 内の版（platform 側の互換範囲指定から解決されたもの。Python の推移依存を含む全パッケージの lock ではありません）：

| package | 観測版 |
| --- | --- |
| framework-arduinoespressif32 | `3.20017.241212+sha.dcc1105b`（Arduino ESP32 2.0.17） |
| toolchain-xtensa-esp32s3 | `8.4.0+2021r2-patch5` |
| toolchain-riscv32-esp | `8.4.0+2021r2-patch5` |
| tool-esptoolpy | `2.40900.250804`（esptool 4.9.0） |
| tool-scons | `4.40801.0` |

## 正式 host test

追加 framework／PlatformIO native platform は不要です。以下をそのまま実行します（assert を無効にする `-DNDEBUG` は付けません）。成功時は `eye_animation: all assertions passed`、終了 code 0。コンパイル警告は `-Werror` により失敗します。

```sh
(
  set -eu
  mkdir -p .test-output
  /usr/bin/clang++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude \
    src/eye_animation.cpp tests/eye_animation_test.cpp \
    -o .test-output/eye_animation_test
  .test-output/eye_animation_test
  rm .test-output/eye_animation_test
  rmdir .test-output
)
```

固定 seed／時刻から、視線範囲と虹彩（両 style とも眼球半径の48%）の収まり、移動中の補間、移動後の静止、まばたきの閉眼・再開眼、32bit wraparound、大きな更新間隔からの復帰を通常の `EyeAnimation(now, seed)`／`update(now)`／`state()` で確認します。`style()`／`toggleStyle()` では漫画風起動、漫画風→リアル風→漫画風の交互切り替え、別インスタンスの漫画風初期化を確認します。移動途中と閉眼中に切り替え、同じ seed の未切替インスタンスと起動後15秒までの状態を比較し、現在状態だけでなく将来の phase・乱数進行も保たれることを確認します。描画 API、実ボタン、実機、乱数分布や全入力の検証ではありません。

## 正式 firmware build と成果物確認

先に `platformio.ini` を読み、自動 upload 等が追加されていないことを確認します。pip 子プロセスの cache も無効化し、Core の保存先はこの実行だけで指定します。初回は toolchain／公式依存を download します。インストール済みでも同じコマンドで再実行できます。

```sh
(
  set -eu
  cat platformio.ini
  export PLATFORMIO_CORE_DIR="$PWD/.cache/platformio"
  export PIP_NO_CACHE_DIR=1 PIP_DISABLE_PIP_VERSION_CHECK=1
  .venv/bin/pio run -e m5stack-stopwatch
  .venv/bin/pio pkg list -e m5stack-stopwatch
  stat -f '%N %z bytes' .pio/build/m5stack-stopwatch/firmware.elf \
    .pio/build/m5stack-stopwatch/firmware.bin \
    .pio/build/m5stack-stopwatch/bootloader.bin \
    .pio/build/m5stack-stopwatch/partitions.bin \
    .pio/build/m5stack-stopwatch/firmware.map
)
```

期待結果は `SUCCESS` と終了 code 0、および空でない成果物です。Issue #2 の最終 firmware ソースの compile／link は成功し、コンパイラ／リンカ警告はありませんでした。出力先は `.pio/build/m5stack-stopwatch/`：

| ファイル | 観測サイズ（bytes） |
| --- | ---: |
| firmware.elf | 14,299,448 |
| firmware.bin | 541,248 |
| bootloader.bin | 15,104 |
| partitions.bin | 3,072 |
| firmware.map | 13,484,145 |

パスや debug 情報等により ELF／map サイズは環境間で変わり得ます。静的 RAM 22,760 / 327,680 bytes、Flash 540,877 / 6,553,600 bytes。これは runtime PSRAM 使用量や FPS の測定ではありません。

## GitHub Actions CI

`.github/workflows/ci.yml` は `main` への push と、`main` 向け Pull Request で次の2つの job を並行実行します。作業ブランチへの push だけでは起動しません。

- `host-test`：Ubuntu 24.04 の `clang++` で上記と同じ C++11／警告設定の host test を compile／実行します。
- `firmware-build`：Ubuntu 24.04 で uv 0.13.5／Python 3.14 の専用 `.venv` を用意し、PlatformIO Core 6.1.19／pip 25.3 を導入して `pio run -e m5stack-stopwatch` を実行します。platform／ライブラリの固定版は `platformio.ini` をそのまま使います。

CI の Python は使い捨ての GitHub-hosted runner 内で必要に応じて取得します。ローカル環境の「既存 Python を明示し、自動取得しない」手順は変えません。Actions 自体は commit SHA に固定し、repository の権限は `contents: read`、checkout の認証情報は保存しません。upload／実機操作、自動 release、成果物の公開、定期実行、full／fast 切り替えは追加していません。

merge の必須条件にするには、workflow を GitHub で一度実行した後、repository の branch ruleset で `host-test`／`firmware-build` を required status checks に指定します。workflow の追加だけでは必須化されません。実機での電源再投入・表示確認は CI の保証対象外です。

## 実装の境界

- `include/eye_animation.h`／`src/eye_animation.cpp` は Arduino 非依存の共通状態。`EyeState` の視線座標は眼球半径比、openness は 0..1。`EyeStyle` は Cartoon／Realistic の2値で、`EyeAnimation` の `style()`／`toggleStyle()` が選択だけを扱います。更新処理は style を参照せず、切り替えは動作状態や時刻・乱数 seed を変更しません。
- 初期静止 1000ms、移動 320ms（smoothstep）、静止 700..1800ms、各軸 ±0.35、blink 待ち 2500..5500ms、閉じる 80ms／閉眼 40ms／開く 120ms。目標・待ち時間を毎フレーム再抽選しません。実機では `esp_random()` を seed に使います。
- `uint32_t(now - phaseStart)` で wraparound を扱い、1回の更新に視線の phase 遷移は最大1回。長い間隔は過去の動きを再生せず現在時刻へ基準を移します。blink 中の大きな飛びは開眼に戻ります。更新が止まっている間の見え方は保証しません。millis の完全な一周以上（約49.7日）更新しない場合の経過周回数は識別できません。
- `src/main.cpp` の `renderCartoonEye(const EyeState&)` は既存の白目の下側のグレーの影、下側が濃い赤の虹彩、小さな黒い瞳孔、右上の薄いピンクのハイライト1つを維持します。漫画風には外枠の黒線・文字・放射状の模様を追加しません。`renderRealisticEye(const EyeState&)` は青緑系の抑えた虹彩、25段階の濃淡、虹彩ローカル座標で固定した72本の放射線、大小の映り込みを図形で描きます。血管や画像素材は使わず、模様をフレームごとに再抽選しません。色・陰影・自然さは実機で最終調整していない初期定数です。
- `renderEye(const EyeState&, EyeStyle)` が同じ状態を選択中の描画へ渡し、まぶた mask と `pushSprite()` を共通化します。描画寸法は画面から取得し、466／468 を固定しません。canvas 一枚を setup で RGB565／PSRAM に一度確保し、フレーム内は再確保しません。失敗時は直接最小のエラー表示を試み、loop で未確保 canvas を使いません。driver 自身の内部 buffer はアプリの追加 canvas とは別です。
- `M5.update()` 直後、33ms の描画間引きより前に `M5.BtnA.wasClicked()` を1回だけ読み、真なら `toggleStyle()` を1回呼びます。固定版の `M5Unified/src/utility/Button_Class.hpp`／`.inl` の公開 API を直接確認しました。`wasClicked()` は `state_clicked` の判定で、`setRawState()` は短押しの release 時だけそれを生成し、次の更新で状態を変えます。既定 hold 閾値は500msで、hold 中・hold 後の release に click はありません。StopWatch の GPIO2→BtnA／GPIO1→BtnB の割り当てと更新は `M5Unified/src/M5Unified.inl` にあります。独自 debounce／repeat や BtnB／電源操作、永続化は追加しません。これらはソース確認であり実ボタン応答の測定ではありません。
- setup の `prepareStopWatchIo()` は内部バス（SDA47／SCL48、100kHz）と公開 PM1 API で必要な3.3V電源を有効にし、IOE1 の `begin()` 成功を最大約3秒まで確認します。5ms の間隔は起動時だけで、成功後は直ちに進みます。利用したバスを解放してから標準の `M5.begin(cfg)` に渡します。失敗時は serial に最小エラーを出して停止し、未初期化の Canvas を使いません。
- 33ms 未満の frame は描画せず、アニメーション待ちの長い `delay`、追いつき loop、engine／plugin、通信／音声／永続化はありません。FPS や転送のティアリング防止を保証しません。

## 証拠と cleanup

各実行者は `.pi/verification-evidence/<run>/` に、自身の source 識別（unborn Git ならその旨と content hash）、dirty 状態、コマンドと終了 code、版、期待／観測、警告、未確認事項を短く残します。秘密値や raw 大量ログは保存しません。今回の worker の記録は `.pi/verification-evidence/worker-issue-2/verification.md`（過去の Issue #1 は `worker-issue-1/verification.md`）。Root は最終差分を review し、同じ host test／build／成果物確認を独立に再実行・記録します。

host test 成功時は上記コマンドが自分の binary と空の出力ディレクトリだけを削除します。失敗時に残った場合も同じ所有物だけを削除してください。host test は background process を起動しません。今回の Issue #2 は background process／serial handle を起動していません。過去の Issue #1 の実機診断で一時起動した90秒上限のログ取得 process は当時停止済みで、serial handle も閉じています。証拠は cleanup 後も保持し、読み取り可能か確認します。

firmware の生成物を clean する場合（任意、USB 操作なし）：

```sh
PLATFORMIO_CORE_DIR="$PWD/.cache/platformio" .venv/bin/pio run -e m5stack-stopwatch -t clean
```

`.venv` と `.cache/platformio` は再利用のため通常は保持します。完全に環境を撤去する場合のみ、他の作業が使っていないことと自分が作成したものかを確認し、当該 `.venv/`、`.cache/platformio/`、`.pio/` だけを削除します。`.cache/` 全体、証拠、既存 docs、共有環境は削除しません。過去の Issue #1 の初回 pip 不足による未完了 tool-esptoolpy と依存解決順変更時の libdeps は、当時の実行者が生成した対象ディレクトリだけを再取得しました。Issue #2 では依存の追加 install／再取得はしていません。

## 電源再投入時の黒画面（過去の Issue #1 の履歴）

以下は当時の許可に基づく過去の調査記録です。Issue #2 では再実行しておらず、以下の操作手順も今回の実行対象ではありません。

症状：USB 接続中でも、電源ボタンの短い1回押しで電源を入れ直すと黒画面になり、再アップロード直後は表示が戻りました。機種固定だけでは解消しませんでした。

診断では通常の serial open が USB reset を起こし、症状を隠すことがありました。DTR／RTS を書き換えない読取で、実電源投入（`reset=1`）でもアプリと PSRAM／Canvas は動いている一方、IOE1 の画面電源・reset 出力が初期状態のままであることを確認しました。M5GFX の `pmic_ops.hpp` の500ms待ちと、`prepare.inl` の失敗後も画面構築へ進む経路が関連します。

起動前に IOE1 の実応答を待つ修正候補では、**応答まで687ms**を観測しました。IOE1 の direction／output に画面電源・reset の必要なビット（`0x98`）が設定され、表示も復帰しました。固定の長い sleep やライブラリ改変ではなく、上限付きの応答待ちをアプリの setup に入れています。診断版の定期ログ・追加 debug flag は最終版から除去しました。

起動修正後の通常版：host test、compile／link、upload・書込領域の hash 確認が成功。serial ポートを閉じた状態で、ユーザーが **USB 接続で2回の電源オフ→オンと、USBを外した電池駆動での電源オフ→オン**を行い、いずれも表示を確認しました。Agent はログと書込結果を確認し、画面の見え方はユーザーの実観測に基づきます。証拠は `.pi/verification-evidence/hardware-boot/`（Git管理対象外）です。その後の2枚目の参考画像に合わせた最新描画版も build／upload し、ユーザーが見た目と電源再投入後の表示を確認済みです（記録は `.pi/verification-evidence/cartoon-reference-2/`）。起動前の応答確認はこの描画変更で変えていません。

再確認時は、許可された対象機器を抜き差し前後のポートと機器情報で照合し、通常の upload に `--upload-port` を明示します。書き込み後は monitor を閉じてから、本体の電源をダブルクリックでオフ→数秒待つ→短押し1回でオンにします。USB 接続と電池駆動を区別し、表示・動きの復帰を確認します。再発時に、monitor 接続による reset だけを成功として扱わないでください。Flash 全消去はこの調査では不要でした。

## 未確認・対象外

Issue #2 の最終ソースは実機未検証です。過去の Issue #1 で1台の漫画風表示・PSRAM／Canvas 確保・電源再投入を確認した履歴はありますが、今回のリアル風表示、実ボタンによる切り替え、最新版での PSRAM 実確保や起動を保証しません。実機 FPS、ティアリング、画面端、色、自然さ、不気味さ、長時間稼働、電池持ち、他個体／温度／低電池状態での起動、IOE1 応答不能時・Canvas 確保失敗時の実機動作も未確認です。host test や compile は描画品質や hardware 統合を保証しません。USB／serial open・実機情報取得・reset・Flash 操作・upload・monitor は今回はゼロです。preview／simulator／E2E／独自 driver／新規検証基盤は追加していません。
