# CardTFT: Wi-Fi SSH と日本語表示

対象は Cardputer ADV (ESP32-S3FN8、8 MB Flash、PSRAM なし) と GMT028-05 V1.1 / ST7789 240×320。外付け画面を横 320×240 で使用します。今回の範囲は Wi-Fi・SSH・日本語表示です。R は SSH サーバーで実行してください。日本語 IME と Tailscale は将来拡張です。

## 環境と来歴

- 作業場所: `/workspace/cardputer_adv_ssh`
- origin: `https://github.com/mokjpn/cardputer_adv_ssh.git`
- upstream: `https://github.com/clown145/cardputer_adv_ssh.git` (今回登録)
- 開始 HEAD: `56d072ae2813e09c49c24bc6200185c3eac4f375`。upstream HEAD と一致。
- 専用ブランチ: `feat/cardtft-japanese-hostkey`
- AGENTS.md / `.agents/skills` の追加指示なし。
- ESP-IDF v5.4.3 (`ea1c174c1cbb7348bd8ba0ff1eb306246938dd80`)、GCC 14.2.0。
- 取得済み依存: M5Unified 0.2.25、M5GFX 0.2.32、libssh2_esp 1.1.0。`dependencies.lock` を追跡して再現性を確保。

初期セットアップの過去ログにはダウンロード 403 がありましたが、今回の再試行で取得できました。ComponentManager のホーム下キャッシュは書込不可だったため `XDG_CACHE_HOME=/workspace/.cache` を使用。別コンピューターには切り替えていません。

## ビルドと切替

このクラウド環境では以下を実行できます。別の開発環境では通常の ESP-IDF `export.sh` を使用してください。

```bash
cd /workspace/cardputer_adv_ssh
source scripts/idf-env.sh
# 内蔵 240×135 (デフォルト、日本語フォント)
idf.py build
# 外付け 320×240 (独立したビルド・設定ファイル)
idf.py -B build-cardtft -DSDKCONFIG=sdkconfig.external \
  -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.cardtft' build
# ホスト自動テスト。C/C++ compiler と OpenSSL 開発ヘッダーが必要
scripts/test-host.sh
```

`menuconfig` の CardTFT で `CARDTFT_EXTERNAL` と `CARDTFT_JAPANESE` を選択します。外付け用 menuconfig は同じ `-B`・`-DSDKCONFIG` 引数を付けて実行してください。中国語フォントに戻す場合は `CARDTFT_JAPANESE` を無効にします。既存 sdkconfig には defaults は上書き適用されません。実行中の画面切替ではなく、ビルド時切替です。

外付け既定値は回転 1、20 MHz、反転 ON、RGB/BGR swap OFF。実パネルで色が違う場合は `CARDTFT_INVERT` / `CARDTFT_RGB_ORDER` を調整します。バックライトの追加 GPIO 制御は設定していません。

## 配線と SPI

| 用途 | GPIO |
|---|---:|
| 外付け LCD SCLK / SD SCLK | 40 |
| 外付け LCD MOSI / SD MOSI | 14 |
| LCD RST | 3 |
| LCD DC | 6 |
| LCD CS | 5 |
| SD MISO | 39 |
| SD CS | 12 |
| TCA8418 SDA / SCL | 8 / 9 |
| TCA8418 INT | 11 |

外付けは SPI2、M5GFX の内蔵 LCD は SPI3。初期化前に SD CS を HIGH にし、`use_lock=true` / `bus_shared=true` で外付けバスを構成。LCD の読み出しは無効。画面全体の RAM framebuffer は作らず LCD GRAM へ描画します。

**この SSH プロジェクトは SD のマウント・ファイル操作を実装していません。** AREA512 のゲーム・SD 機能は移植していません。今回の静的検証はピン・SPI ホスト分離・非選択 CS・ロック設定までです。将来 SD を使う際は同じ SPI2 を再初期化せず共有し、LCD/SD の排他制御と同時 I/O を実機検証してください。

## 書き込み (ユーザーが実行)

この作業では実機への書き込みは行っていません。`PORT` はユーザーの実機ポートに置き換えます。

```bash
# ソースから外付け版を書き込む
idf.py -B build-cardtft -DSDKCONFIG=sdkconfig.external -p PORT flash monitor
# 内蔵版に戻す
idf.py -p PORT flash monitor
```

成果物パッケージの各 `external/` または `internal/` ディレクトリには `flash_args` と必要な 3 イメージがあります。そのディレクトリで ESP-IDF 環境の Python を使います。

```bash
python -m esptool --chip esp32s3 -p PORT -b 460800 \
  --before default_reset --after hard_reset write_flash @flash_args
```

アプリ .bin 単独をアドレス 0 に書き込まないでください。付属 flash_args は bootloader=0x0、partition table=0x8000、app=0x10000 を指定します。NVS のイメージは含まず、既存保存内容の消去は行いません。全 Flash 消去はホスト鍵の信頼履歴も失います。

ROM ダウンロードモードが必要なら、背面 BtnG0 を押しながら BtnRST を押して離し、約 1 秒後に BtnG0 を離します。


## 書き込み前のイメージ照合 (必須)

対応ボードは **M5Stack Cardputer ADV / ESP32-S3FN8 / 8 MB Flash / PSRAM なし** です。通常の Cardputer や異なる Flash 容量向けのイメージではありません。

| パッケージ内ディレクトリ | 接続する表示器 |
|---|---|
| `external/` | GMT028-05 V1.1、ST7789 240×320、横 320×240。本文の配線が必要 |
| `internal/` | Cardputer ADV 内蔵 LCD 240×135 |

| アプリイメージ | SHA256 | bytes |
|---|---|---:|
| 外付け ST7789 / 日本語 | `0fa306424dc2f8955c0f01f8d60ebbef75b4be5d87fb3e2a0fdb951b0ec3d195` | 2509440 |
| 内蔵 LCD / 日本語 | `1df480057e3826a19f8a6bd663d4e15d57898e577f7ff25f557b16e2e850cb3c` | 2509040 |

パッケージのルートで `sha256sum -c SHA256SUMS` を実行し、選択した構成の全ファイルが OK であることを確認してください。構成を混ぜず、選択したディレクトリの `flash_args` を使用します。

| 書き込むファイル | Flash オフセット |
|---|---|
| `bootloader/bootloader.bin` | `0x0000` |
| `partition_table/partition-table.bin` | `0x8000` |
| `cardputer_adv_ssh.bin` | `0x10000` |

NVS の書き込みは含まれません。アプリ .bin をオフセット 0 に書き込まないでください。

実行済みビルドコマンド (プロジェクトルート、`source scripts/idf-env.sh` 後):

```bash
# 外付け版
idf.py -B build-cardtft -DSDKCONFIG=sdkconfig.external -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.cardtft' build size
# 内蔵版
idf.py build size
# 自動テスト
scripts/test-host.sh
```

自動テストは **4 本のテスト実行ファイル、4 本成功・0 本失敗** (ASan/UBSan 有効)。terminal、host-key policy、SSH auth gate + NVS adapter、Japanese efont の 4 本です。ループ内の assertion 数を「テスト件数」に加算していません。フォント検査は別指標として 4 種×65,536 = 262,144 回の BMP 検索を行っています。詳細は `validation.md` を参照してください。

## 接続と操作

1. 本体 Wi-Fi メニューでユーザー自身が接続を設定。
2. SSH Profiles で `user@host[:port]` を登録。
3. Terminal を開く。初回はホスト・ポートと SHA256 フィンガープリントを表示。長いホスト名は Enter でページ送り。
4. 別の信頼できる経路でサーバー管理者から確認した値と照合。サーバー側の例: `ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub -E sha256`。交渉された鍵の種類が異なる場合は対応するホスト公開鍵を確認。
5. 一致した場合だけ Enter の後の確認画面で **Shift+Y (大文字 Y)**。それ以外のキーは拒否。フィンガープリントが NVS に保存・commit されてから認証を開始。

保存先は専用 NVS namespace `ssh_hosts`。ホスト (ASCII 小文字化) とポートに紐付け、プロファイル名・ユーザー名の変更で信頼をリセットしません。索引は SHA256 の一部ですが、保存した完全なホスト・ポートも検証し、衝突・破損時は拒否します。保存済み鍵と異なる鍵は **HOST KEY CHANGED - refused** として拒否し、上書き確認には進みません。鍵更新 UI は未実装です。正当なサーバー鍵更新も管理者による確認と明示的なメンテナンスが必要です。

ホスト鍵確認前には公開鍵認証の問い合わせもパスワードも送りません。確認コールバックがない接続経路は未知の鍵を拒否します。検証無効化・Telnet fallback はありません。NVS 初期化エラー時にも自動消去せず、画面に Storage error を出して起動処理を停止します。

- Esc: 履歴表示。Esc を素早く 2 回: ランチャーへ。
- 履歴表示中 Up/Down: スクロール、Left/Right: 一時ズーム、Enter: 現在画面へ。
- Fn+`; , . /`: 上・左・下・右。Fn+文字: Ctrl+文字。Fn+`[`: Esc。
- Terminal Settings の CJK 10/12/12 Bold/14 は、日本語ビルドでは日本語フォントです。
- 標準の外付け Compact + Full chrome は **53 桁×18 行**、内蔵は **40 桁×9 行**。PTY サイズも追従。

`tests/manual_terminal.py` を SSH サーバー側で UTF-8 locale にて実行し、日本語・半角カナ・右端全角折返し・ANSI 色・連続スクロールを確認してください。

## 日本語とメモリの制約

libvterm の UTF-8 / 2 セル幅処理を使用。ASCII に続く UTF-8 の途中で受信が分割されるケースを修正し、全バイト境界をテスト。フォント検索は日本語 efont の 4 種を利用します。半角カナは同フォントに収録されていないため、対応する全角字形を横方向に縮小して 1 セルへ描画します。収録外文字は `?`。結合文字の合成・絵文字・非 BMP 漢字の表示は未対応です。

日本語フォントの Flash データ合計は **1,226,036 bytes** (10:225686、12:308265、12 Bold:308338、14:383747)。Flash 上で参照し、全体を RAM に展開しません。中国語版との両方は同時にリンクしません。

履歴は最大 64 行、53 桁の場合は packed cell データ最大約 27 KB。従来の 48 KB 空き heap と最大連続ブロックのガードも維持。画面スナップショット・libvterm の通常／代替画面・Wi-Fi・SSH の動的割当は別に必要です。ターミナル開始時に UART ログへ grid・heap free・largest block・minimum free を出力します。静的リンク成功は実通信時の RAM 安定性の保証ではありません。

## 検証の境界と実機チェックリスト

クラウドでの結果は `validation.md` と成果物内 `evidence/` を参照してください。以下は **未検証** です。

- [ ] 実配線、電源・バックライト、横 320×240 の四隅・回転・色順・反転・欠け。
- [ ] 内蔵版の既存メニュー・キーボード・Wi-Fi・SSH の回帰確認。
- [ ] TCA8418 (GPIO8/9、INT11)、Fn/Shift、初回確認で大文字 Y が入力できること。
- [ ] SD を挿した状態で LCD に乱れがないこと。SD 同時 I/O は今後の実装後に別途検証。
- [ ] 実 SSH サーバーとの鍵交渉、初回拒否でサーバー認証ログに試行がないこと。
- [ ] 初回受諾→切断→再接続→電源断→再接続で、保存済み鍵が一致すること。
- [ ] サーバー鍵を検証用環境で変更したとき、認証を送らず拒否すること。
- [ ] 長いホスト名、IPv6、異なるポート、NVS 満杯・破損時の拒否表示。
- [ ] UTF-8 日本語、半角カナ、全角の右端折返し、改行、消去、履歴、ズーム、代替画面、R の出力。
- [ ] 長時間スクロール、Wi-Fi 再接続、SSH 再接続、WebUI 併用時の heap と最大ブロック、ウォッチドッグ。

日本語 IME は候補辞書・RAM・入力 UI の設計が別途必要です。Tailscale は今回搭載せず、将来ネットワーク構成とメモリ予算を検討します。

## ライセンス

本体は GPLv3 (`LICENSE`)。efont は BSD-3-Clause、M5GFX/M5Unified は MIT、libvterm は MIT、libssh2 は同梱 COPYING、libssh2_esp は BSD-3-Clause。通知を `docs/licenses/` に収録しました。LovyanGFX 由来部分にはそのソース中の BSD 通知が適用されます。ESP-IDF と依存の元ソースの通知も維持してください。

AREA512 からコード・フォントデータをコピーしていません。ピンはユーザー提供の確認済み配線に基づき、既存の M5GFX API と同梱 efont を利用しました。将来 AREA512 の実装を取り込む場合は、その時点で対象ファイルのライセンスを確認してください。
