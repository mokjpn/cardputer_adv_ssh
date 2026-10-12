# クラウド検証結果 (2026-10-12)

実機・実ネットワーク・物理 NVS は未検証。元ファームウェアと変更後の両画面構成が ESP-IDF v5.4.3 でビルド成功。

| 構成 | アプリ bin (bytes) | アプリ領域の残り (bytes) | SHA256 |
|---|---:|---:|---|
| 外付け ST7789 / 日本語 | 2509440 | 1029504 | `0fa306424dc2f8955c0f01f8d60ebbef75b4be5d87fb3e2a0fdb951b0ec3d195` |
| 内蔵 LCD / 日本語 | 2509040 | 1029904 | `1df480057e3826a19f8a6bd663d4e15d57898e577f7ff25f557b16e2e850cb3c` |

各 app partition は 0x360000 (3,538,944) bytes。8 MB Flash・PSRAM 無効を両 sdkconfig.h で確認。

## 自動テスト

`scripts/test-host.sh` を ASan / UBSan 有効で実行。テスト実行ファイル 4 本、成功 4・失敗 0。ループ内 assertion 数はテスト件数に加算しません。BMP 全検索は 4 種×65,536 = 262,144 回。

```text
terminal: split UTF-8, CJK width, wrap, CRLF, scrollback cap, ANSI, alternate screen, resize PASS
host policy: reject, accept/save, repeat, changed key, port isolation, read/write failure, no callback PASS
SSH auth gate + NVS adapter: fingerprint vector, reject before either auth, reconnect, changed key, renamed profile, port, storage error/corruption PASS
efont: four Japanese presets, kana/kanji/punctuation coverage and all BMP lookups PASS (43353 glyphs)
```

SSH テストは実装の `SshClient::connect` と NVS アダプターを実行しますが、libssh2/接続はモック、NVS はメモリ上の代替、SHA256/base64 はホスト OpenSSL です。実サーバー認証・ESP 上の暗号実装・電源断保存を検証したものではありません。フォントテストは実際の日本語 efont データと本番の検索関数で代表日本語・半角カナ対応・BMP 全検索を検査。ピクセルの見え方は実機確認が必要です。

## リンク時メモリ

```text
external
│ DIRAM             │       126551 │    37.03 │         215209 │        341760 │
Total image size: 2509324 bytes (.bin may be padded larger)
internal
│ DIRAM             │       126023 │    36.87 │         215737 │        341760 │
Total image size: 2508916 bytes (.bin may be padded larger)
```

DIRAM の空きはリンク時の値です。起動後に Wi-Fi・SSH・libvterm が確保するメモリを差し引いた free heap ではありません。実機のログと長時間操作で確認してください。

## ライセンスと SPI

- efont の BSD-3-Clause 通知を確認して同梱。AREA512 からの転用なし。
- LCD は SPI2、内蔵は依存ライブラリで SPI3。SD CS12 は初期化時 HIGH、MISO39、共有ロック有効。SD は未マウントで、同時 I/O は未検証。
- 日本語 4 フォントだけが Flash アドレス 0x3c... にリンクされ、Chinese フォントのシンボルは未リンク。データ合計 1,226,036 bytes。

## 未実施・残課題

`cardtft-ja.md` の実機チェックリストを参照。初回ビルド検証時点では、クラウドから実機書込・ユーザーのネットワーク設定変更・認証情報の保存取得・push・PR は実施していません。ソースの push・ドラフト PR は、その後の個別承認で別途実施します。ビルドを妨げる残存障害はありません。

## 実行コマンドと対象

対象は M5Stack Cardputer ADV / ESP32-S3FN8 / 8 MB Flash / PSRAM なし。外付け版は GMT028-05 V1.1 (ST7789)、内蔵版は ADV 内蔵 LCD。

```bash
source scripts/idf-env.sh
idf.py -B build-cardtft -DSDKCONFIG=sdkconfig.external -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.cardtft' build size
idf.py build size
scripts/test-host.sh
```

Flash オフセットは bootloader `0x0`、partition table `0x8000`、app `0x10000`。SHA256 は上表。実機書き込みは未実施です。

## 配布前の資格情報・パッケージ検査

2026-10-12、配布 ZIP と内包 source.zip の 121 ファイルを検査。PEM 秘密鍵ブロック・代表的な API/アクセスキー形式の検出 0、`.git` / `.ssh` / `.aws` / `.env` / `auth.json` / `credentials` / `nvs.bin` の混入 0。非空の資格情報リテラルはテスト用の `synthetic` パスワード 1 件のみで、実ユーザーの資格情報ではありません。コードと梱包手順の確認でもユーザー資格情報の追加・収集は行っていません。これは既知パターンと梱包対象の検査であり、あらゆる秘密形式を数学的に証明する検査ではありません。

両構成のイメージを生成元 build とバイト照合し、ZIP の CRC、SHA256SUMS、8 MB / PSRAM 無効 / 外付け・内蔵設定、flash_args のオフセットを確認しました。確認のための実機接続・書き込みはしていません。
