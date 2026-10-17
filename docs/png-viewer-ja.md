# SSH PNG ビューアー

Cardputer ADV + Cap TFT V2 外付け ST7789 (横320×240、PSRAM無効) 用のSSH PNGビューアーです。

## 操作

1. 通常のホスト鍵確認と認証を経て Terminal へ接続。
2. Esc で Review、`p` で PNG viewer。SSH に `p` は送信しません。
3. サーバーの絶対パスを入力して Enter。相対パス、制御文字、512 bytes超は拒否。
4. 取得後、画像を画面中央に等倍表示。Enter / Esc でターミナルの再描画に戻ります。
5. 入力中 Esc は取消。取得・デコード中 Esc は中断。取得失敗・30秒超過・256KiB超過はセッションを破棄し、Terminal から再接続してください。デコード失敗では接続を維持し、Retry remote pathでパスを再入力できます。

PNG専用の `png_cache` パーティション（0x710000、960KiB）だけを使います。既存 `storage`・NVSの位置とサイズは維持し、マウント・初期化・消去しません。PNGキャッシュのマウントに失敗した場合はエラー番号を表示して戻ります。本体からフォーマットする操作はありません。通常終了時・次回起動時のビューアー開始で専用一時ファイル `/images/ssh-view.tmp` を削除します。保存画像の一覧・永続キャッシュはありません。

移行時は末尾0x710000〜0x800000が未割当で全FFであることをFlashバックアップで確認し、パーティション表・アプリ・専用キャッシュの空SPIFFSイメージだけを投入します。全消去や既存storageのイメージ書込みはしません。CMakeが現在のSPIFFS設定から専用キャッシュの空イメージを生成します。通常の `idf.py flash` もこの使い捨てキャッシュを再作成します。既に利用中のキャッシュを保持したい場合は、アプリだけを更新してください。起動ログ `PNG cache: ESP_OK total=... used=...` でマウントを確認できます。

## サーバー画像の例（ユーザーが実行）

Linux/POSIX シェルと `cat --` が使えるサーバーが対象。追加サーバーデーモン、SCPコマンド、SFTPサブシステムは不要です。ImageMagick 7 の例:

```sh
magick input.png -auto-orient -thumbnail '320x240>' -background black \
  -alpha remove -alpha off -depth 8 -interlace none PNG24:/tmp/cardputer.png
wc -c /tmp/cardputer.png
```

256KiB以下、幅1〜320、高さ1〜240、非インターレース、8bit以下。内蔵画面ビルドでは240×135以下。自動縮小・アニメーション・16bit PNGは対象外。透明部分は黒へ合成。

## 転送と保存

libssh2_esp 1.1.0 の公式配布 CMakeLists は scp.c / sftp.c を含みますが、本体にはファイル転送 API がありませんでした。既存 exec() は出力を無制限に string に蓄積するため使用しません。新しい bounded exec は認証済み・ホスト鍵検証済みセッションに別チャネルを開き、引用したパスの `cat -- '…'` を実行。引用符もエスケープし、stderrは破棄、終了ステータスを確認します。PNGをシェルPTYに混ぜません。転送中の既存ターミナル出力は処理し、履歴上限を維持します。

SDは未マウントのまま。LCD/SDはSPI2、SCLK40/MOSI14/MISO39、LCD CS5/DC6/RST3、SD CS12。SPI共有の新規実装を避け、983,040-byteの専用SPIFFS png_cache領域を使用。空き320KiB以上を要求し、保存完了・flush/close成功後のみデコード。電源断時の部分ファイルは表示しません。

PNGdec公式ソース https://github.com/bitbank2/PNGdec を components/pngdec に固定同梱。commitは UPSTREAM、Apache-2.0は LICENSE。READMEの最低約48KB・行コールバック・非インターレース制約に従います。全体CRC/構造/寸法/終端検査を先に行い、さらにPNG_CHECK_CRCでデコード。全画面RGBバッファ(153,600 bytes)は確保しません。

## メモリー予算

- PNG作業領域（分割版）: ESP32-S3実ビルドで本体13,024 bytesと8,192 bytes×4ページ、合計45,792 bytes。32KiBのDEFLATE辞書容量は維持。RGBA用の行作業領域はフィルターと整列余白を含む2,592 bytes。転送と重ねず保存後だけheapへ確保し、表示待ち前に全て解放。
- RGB565描画行: 640 bytes、コンテキスト・ヘッダー込み約0.8KiBのスタック。
- 転送バッファ: 1,024 bytesのスタック、パス最大512 bytes。画像チャネルは受信window16KiB/packet4KiB。ファイル全体のRAM保持なし。
- デコード開始には合計78,560 bytes以上の空きheapと13,024 bytes以上の最大連続ブロックを要求。分割確保後も実空き32KiB以上を再確認。不足時・ページ確保失敗時は全解放して拒否。開始/終了後のheap・largest・minimumをUARTログで確認できる。
- SPIFFSの作業領域・libssh2チャネル/暗号・Wi-Fi・既存terminalの割当は別。これは全アプリのRAM上限保証ではありません。実機で最悪条件のfree/largest/minimum heapを確認する必要があります。画像表示中はDecoderを解放済み。デコード中は一時heap減少で履歴を追い出さないようshell出力の取り込みを保留し、解放後の画面復帰で再開します。SSH出力はこの間backpressureがかかり得ます。履歴上限・端末画面・SSH接続機能は削減していません。

## 再現と実機確認

```sh
source scripts/idf-env.sh
idf.py -B build-cardtft -DSDKCONFIG=sdkconfig.external \
  -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.cardtft' build size
scripts/test-host.sh
scripts/test-image.sh
```

ホスト試験: 既存terminal/host policy/auth gate/Japanese fontに加え、mock transportによる上限・空・exit失敗・取消・timeout・再接続、PNG実デコード、全バイト位置の切詰め/1bit破損をASan/UBSanで確認。ネットワーク実接続を行う試験ではありません。

実機未確認: SPIFFS初期化/満杯/電源断復旧、320×240の四隅・RGB/alpha、通常SSH/日本語/鍵照合の回帰、並行大量シェル出力、Wi-Fi切断/転送中Esc/連続再試行、最小heap・断片化・watchdog。鍵生成・サーバー設定変更は不要です。

## クラウド検証結果 (2026-10-16)

ESP-IDF 5.4.3 / esp-14.2.0_20250730、Flash 8MB、PSRAM無効、外付け構成でbuild size成功。ホスト回帰5実行ファイルと画像1実行ファイルが成功、ASan/UBSan報告なし。内蔵構成の今回のビルドと全構成の実機確認は未実施。

初回環境はrich等のPython依存不足を修復。開発途中の並行ビルドによる中間オブジェクト競合も解消し、最後は単独ビルドで成功を確認しました。これらの途中ログは最終成功の根拠には使用していません。


## PNGパスのセッション履歴（png-history-20261017）

パス入力を開くと直前にEnterで送信したパスを表示します。Ctrl-Pで古い履歴、Ctrl-Nで新しい履歴へ移動し、最新を越えると編集中の文字列へ戻ります。最古でCtrl-Pを押しても止まります。編集後にCtrl-Pを押すと、編集内容を一時退避して最新履歴から辿ります。Enterで送信・記録、Escで入力取消です。失敗した取得のパスも再編集できます。空文字列と連続重複は記録しません。

履歴はRAM内のみで、最大8件、各512 ASCII文字、保持文字列のcapacity合計（終端含む）は1024 bytes以下です。容量超過時は古い順に破棄します。再起動で消え、設定・鍵・NVSには書き込みません。長いパスは末尾を表示します。その他の入力と秘密入力は従来の96文字上限で履歴を使いません。

ホスト試験は入力モデルを直接駆動して再表示、取消、境界、ドラフト復帰、秘密入力分離、10000回の容量制限を確認します。この版は実機へ書込み、全領域読戻しによるアプリ一致・他領域不変、起動・PNG cache認識・Wi-Fi接続を確認しました。ユーザーが画像表示と履歴版の実機動作に問題がないことを確認済みです。四隅・全色、通信中断、連続使用時heap、日本語入力・SSH復帰の個別項目は未完了です。


## ローカル検証（2026-10-18）

ESP-IDF 5.4.3外付け構成、版名png-history-20261017でアプリ2,574,416 bytes、SHA256 `e2dac4db9b9c31fc20f521de42910223502cac8b4f231f5e05ff3de45f0cf512`。これは実機へ書き込んだバイナリの識別値です。GitHub CIの既定構成・版文字列によるバイナリとは異なります。

`bash scripts/test-host-macos.sh`（既存回帰5本）、`test-path-history.sh`、`test-image.sh`、`test-png-file.sh`、`test-png-corpus.sh`、`test-png-allocation.sh` がASan/UBSanで成功。コーパスは13画像×RAM/FILEの26デコードと不正DEFLATE、辞書の境界・コピー・解放を検証し、通常一体辞書版との画素結果一致も確認しました。秘密を含む実機ログ・Flashバックアップ・ローカル設定・ビルドバイナリはリポジトリに含めません。
