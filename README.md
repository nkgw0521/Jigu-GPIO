# Jigu-GPIO

STM32 NucleoおよびDiscoveryボード向けの、SCPIで制御するGPIO・PWMジェネレーターです。

## 対応ボード

| ボード | MCU | 対応状況 |
|---------|---------|---------|
| NUCLEO-F303K8 | STM32F303K8 | 統合済み。Debug/Releaseビルド確認済み、実機試験待ち |
| NUCLEO-L432KC | STM32L432KC | 対応済み |
| NUCLEO-L053R8 | STM32L053R8 | 対応済み |
| NUCLEO-L073RZ | STM32L073RZ | 対応済み |
| STM32L0538-DISCO | STM32L053C8 | 対応済み |

---

## Releaseファームウェアのバイナリ

実機に書き込んだFWの版番号は `*IDN?` の応答の第4フィールドで確認できます。応答の形式は `MANUFACTURE,INSTR2013,0,NN-NN` です。版番号は全ボード共通で、現行リリースの版番号はマニフェストに記録しています。

全対応ボードの現行ReleaseバイナリをGitで管理しています。ファイル名は現在のビルド成果物名に合わせています。L432の旧ファイル `Jigu-GPIO.bin` は `Jigu-GPIO_L432.bin` に置き換えました。

| ボードフォルダー | Releaseバイナリ |
|---------|---------|
| NUCLEO-F303K8 | `Release/NUCLEO-F303K8.bin` |
| NUCLEO_L432KC | `Release/Jigu-GPIO_L432.bin` |
| NUCLEO_L053R8 | `Release/Jigu-GPIO_L053.bin` |
| NUCLEO-L073RZ | `Release/NUCLEO-L073RZ.bin` |
| STM32L0538-DISCO | `Release/Jigu-GPIO.bin` |

リポジトリ直下の `Release-Firmware.ps1` をWindows PowerShell 5.1以降で実行すると、版番号を自動で繰り上げ、5ボードのRelease BINとマニフェストを更新します。

```powershell
.\Release-Firmware.ps1
```

既定のSTM32CubeIDEの場所は `C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE` です。別の場所にインストールしている場合は、`-CubeIdePath` に `stm32cubeidec.exe` のあるフォルダーを指定してください。Gitと `tar.exe` が必要です。

ビルドには作業ツリーのFWソースを使い、一時フォルダーで各BINの版番号とELFから変換したBINとの一致を検証します。5ボードすべての検証が通ると、版番号・BIN・[マニフェスト](tests/release-binaries.json) が反映されます。マニフェストにはベースコミット、変更されたビルド入力のSHA-256（削除は `null`）、各BINのサイズとSHA-256を記録します。失敗時は一時フォルダーにログが残ります。リリース差分の確認後、コミットとGitタグの付与は手動で行います。

## プロジェクト構成

アプリケーションコードと、ボードごとにSTM32CubeMXで生成するコードを分離しています。

```text
App/
  Inc/
    board_config.h
  Src/

Boards/
  NUCLEO-F303K8/
    Core/
    Drivers/
    STM32F303K8TX_FLASH.ld
    NUCLEO-F303K8.ioc

  NUCLEO_L432KC/
    Core/
    Drivers/
    STM32L432KCUX_FLASH.ld
    Jigu-GPIO.ioc

  NUCLEO_L053R8/
    Core/
    Drivers/
    STM32L053R8TX_FLASH.ld
    Jigu-GPIO.ioc

  NUCLEO-L073RZ/
    Core/
    Drivers/
    STM32L073RZTX_FLASH.ld
    NUCLEO-L073RZ.ioc

  STM32L0538-DISCO/
    Core/
    Drivers/
    STM32L053C8TX_FLASH.ld
    Jigu-GPIO.ioc

libscpi/
  Inc/
  Src/
```

アプリケーションの処理は `App/` 以下で共通化し、CubeMXが生成するリソースは各ボードのディレクトリに配置しています。

---

## SCPIコマンド

| コマンド | 値 | 説明 |
|----------|-------|-------------|
| SYSTem:LED | 0: 消灯 / 1: 点灯 | オンボードLEDを制御 |
| GPIO:SELect | 1 | GPIOチャンネルを選択 |
| GPIO:SELect? | - | 選択中のGPIOを問い合わせ |
| GPIO:DIRection | IN / OUTPushpull / OUTOpendrain | GPIOの入出力方向を設定 |
| GPIO:DIRection? | - | GPIOの入出力方向を問い合わせ |
| GPIO:LEVel | 0: Lo / 1: Hi | 出力レベルを設定 |
| GPIO:LEVel? | - | 出力レベルを問い合わせ |
| GPIO:PULl | No / Down / Up | プル抵抗を設定 |
| GPIO:PULl? | - | プル抵抗の設定を問い合わせ |
| GPIO:Speed | Low / Medium / High / Veryhigh | GPIOの速度を設定 |
| GPIO:Speed? | - | GPIOの速度を問い合わせ |
| PWM:Frequency | 0 - 4294967295 | PWM周波数（Hz）を設定 |
| PWM:Frequency? | - | 周波数を問い合わせ |
| PWM:Width | 0 - 4294967295 | パルス幅（us）を設定 |
| PWM:Width? | - | パルス幅を問い合わせ |
| PWM:SYNC | 0: 変更時に停止 / 1: 同期更新 | PWM設定の更新動作を選択 |
| PWM:SYNC? | - | 同期更新モードを問い合わせ |
| PWM:POLarity | 0: アクティブLow / 1: アクティブHigh | 出力極性を設定 |
| PWM:POLarity? | - | 極性を問い合わせ |
| PWM:STart | 0: 停止 / 1: 開始 | PWM出力を開始・停止 |
| PWM:STart? | - | 開始状態を問い合わせ |
| PWM:Counter | 0 - 4294967295 | 外部パルスカウンターの値を設定 |
| PWM:Counter? | - | 現在のカウンター値を取得 |
| PWM:Numbers | 0 - 4294967295 | 出力パルス数を設定（0 = 連続出力） |
| PWM:Numbers? | - | 設定されたパルス数を問い合わせ |
| PWM:DEBUG? | - | PWM内部のデバッグ情報を取得 |

---

## PWMの動作

### PWM出力

- ハードウェアタイマーでPWM出力を生成します。
- 停止制御用のパルス数はタイマー割り込みでカウントします。
- 外部パルスのカウントには、ボードごとに独立したタイマーを使用します。

### PWM:SYNC

`PWM:SYNC` は、周波数・パルス幅・極性の設定を変更したときの動作を選択します。
電源投入時の初期値は `0` です。

| 値 | 動作 |
|---------|---------|
| 0 | 周波数・パルス幅・極性を変更するとPWM出力を停止（従来の動作） |
| 1 | 連続出力を停止せずに周波数・パルス幅の変更を反映 |

`PWM:SYNC 1` の場合：

- 受け付けた周波数とパルス幅を、次のPWM周期境界で同時に反映します。出力中のパルスは途中で切り詰めません。
- 周波数またはパルス幅の各コマンドは、もう一方の受け付け済み設定値との組み合わせで検証します。制御側は、有効な組み合わせになる順序で設定を送信してください。
- 周波数とパルス幅の組み合わせが無効な場合、設定を無視し、既存のSCPIエラーキューに `Execution error` を追加します。
- PWM出力中の極性変更は無視し、`Execution error` を追加します。極性はPWM停止中に変更できます。
- 有限パルス出力中（`PWM:Numbers > 0`）の周波数・パルス幅・極性の変更は無視し、`Execution error` を追加します。
- 拒否した設定は、対応する問い合わせコマンドの返値に反映しません。
- `PWM:SYNC` 自体の変更では、PWM出力を停止・再開しません。

拒否された設定によるエラーは、`SYSTem:ERRor?` で確認できます。

### PWM:POLarity

| 値 | パルス中 | パルス間・停止中のアイドル出力 |
|---|---|---|
| 0（アクティブLow） | Low | High |
| 1（アクティブHigh） | High | Low |

- 電源投入直後、MCUがPWM端子を駆動するまでの間は、外部プルアップでHighになることがあります。ファームウェアの起動処理でPWM端子をGPIO出力に切り替え、初期値の正極性（`PWM:POLarity 1`）に対応するLowを駆動します。初回のPWM開始前からLowを保持します。
- PWM停止中に `PWM:POLarity` を設定すると、`PWM:SYNC` が `0` と `1` のどちらでも、次回の開始を待たずにアイドル出力が切り替わります。この操作でPWMパルスは発生しません。
- 外部にプルアップ抵抗があっても、端子がLowを駆動している間は、抵抗を通じて電流が流れ、端子はLowを保持します。

以下は `PWM:Numbers 3` のとき、パルス生成開始後に端子から出力するレベルだけを描いた図です。開始前のレベルは省略しています。パルス幅と周期の比率は説明用です。

```text
pol=0 / n=3（アクティブLow）

Hi       ┌────────┐      ┌────────┐      ┌────────────
         │        │      │        │      │
Lo ──────┘        └──────┘        └──────┘
   開始                                  以後Hi保持


pol=1 / n=3（アクティブHigh）

Hi ──────┐        ┌──────┐        ┌──────┐
         │        │      │        │      │
Lo       └────────┘      └────────┘      └────────────
   開始                                  以後Lo保持
```

最初のパルスは、`PWM:POLarity 0` ではLow、`PWM:POLarity 1` ではHighです。3発目のパルス終了後は非アクティブレベルを保持し、4発目は出力しません。最後に非アクティブレベルへ戻る遷移は正常な波形の一部であり、グリッチではありません。

停止時は、共通ポート実装が設定されたPWM端子をGPIO出力に切り替え、非アクティブレベル（アクティブLowならHigh、アクティブHighならLow）を保持します。次回の開始時は、GPIOでそのレベルを保持したまま、実際に使用するCCRとプリロードCCRをともに0にしてタイマー出力を有効化し、その後にオルタネートファンクションモードへ切り替えます。指定されたパルス幅は、この切り替えが完了してから、同期したUGイベントで反映します。これにより、TIM21にはない高機能タイマーのアイドル状態機能にも依存しません。

### パルス数制御（PWM:Numbers）

- `PWM:Numbers = 0`
  - 連続出力します。

- `PWM:Numbers >= 1`
  - 指定した数のパルスを出力して停止します。
  - `PWM:Numbers 1` は、全ボード共通の最初の比較一致で停止する処理により、正確に1発を出力します。従来、境界条件で2発出力される動作は修正済みです。
  - 次のPWM周期のパルスをハードウェアで抑止するため、割り込みの遅延による短い余分なパルスを防ぎます。
  - 繰り返し開始する場合も、出力開始前に設定されたパルス幅を再ロードします。
  - 出力チャンネルは、まず非アクティブ状態のシャドーCCRで開始します。その後、共通のUGイベントでタイマーのリセットと実際のパルス幅のロードを同時に行い、HALによる出力有効化の遅延が最初のパルス幅に影響しないようにします。
  - 初期化時のソフトウェア更新イベントは周期数に含めません。開始関数から戻る前に単発パルスが終了しても、比較一致フラグを保持します。
  - 実際のパルス数は、各ボードの外部カウンターで確認できます。

---

## ハードウェア

### NUCLEO-F303K8

| ボード上の表記・コネクター | MCU端子 | ファームウェアの機能 |
|-----|-----|----------|
| D9 / CN3 12番端子 | PA8 (FT) | トリガー／PWM出力（TIM1_CH1）、3.3 V出力 |
| D11 / CN3 14番端子 | PB5 (FT) | 外部パルスカウンター（TIM3_CH2）、ソフトウェアで32ビットに拡張 |
| D2 / CN3 5番端子 | PA12 (FT) | GPIO1 |
| D13 / CN4 15番端子 | PB3 | オンボードLED（LD3）、`SYSTem:LED` の機能を復元済み |
| ST-LINK VCP（SB2開放時はA7） | PA2 | USART2 TX |
| ST-LINK VCP（Nanoヘッダーには未接続） | PA15 | USART2 RX |
| GND / CN3 4番端子またはCN4 2番端子 | GND | 信号グラウンド |

- トリガー先またはオシロスコープを **D9（PA8）** と **GND** に接続してください。
- `PWM:Counter?` で出力数を確認する場合は、**D9（PA8）とD11（PB5）をジャンパー接続**してください。`PWM:Numbers` による有限パルス出力はPWMタイマー内部の割り込みを使用するため、このジャンパーは外部でパルスを計測する場合だけ必要です。
- **SB15は接続したままで使用できます**。D13/PB3はLED出力に戻しており、カウンター入力には使用しません。
- D11/PB5とD2/PA12は5 V耐性のあるデジタル入力です。外部から5 V信号を入力する際は、ボードに給電し、GPIO入力モード・プル抵抗なしで使用してください。プッシュプル出力は3.3 Vです。以前使用していたA1/PA1とA5/PA6は、これらの機能では使用しません。
- TIM3の16ビットカウンターを更新割り込みで符号なし32ビットに拡張しています（0～4,294,967,295、その後0に戻ります）。読み取り時は未処理のオーバーフローも反映し、書き込み・リセット時は上位・下位の両方を更新します。停止・開始をまたいでカウント値を保持します。入力65,536パルスごとに少なくとも1回は割り込み処理が必要です。割り込みが処理できない間に複数回オーバーフローした場合は復元できません。実機で信頼性を確保できる最大入力レートは未測定です。
- ST-LINK USB仮想COMで制御します。通信条件は **115200 baud、8N1、フロー制御なし**、コマンドの終端はLFです。VCPはD1/D0ではなく、PA2/PA15とSB2/SB3を使用します。
- F303は内部クロックHSI/2 ×16 = **64 MHz** を使用するため、外部クロックやクロック用はんだブリッジの変更は不要です。タイミング精度は内部RC発振器に依存します。COM16とD9–D11のループバック試験で、有限パルス出力・単発出力・32ビットカウントを確認済みです。両極性について、単発および3発の出力開始時に、最初のパルス幅1～500 usをタイマークロックに対して確認しました。独立した測定器による周波数とパルス幅の絶対精度は未測定です。

参考資料：[ST DS9866：端子機能とタイマー機能](https://www.st.com/resource/en/datasheet/stm32f303k8.pdf)、[ST UM1956：表12とはんだブリッジ](https://www.st.com/resource/en/user_manual/dm00231744-stm32-nucleo32-boards-mb1180-stmicroelectronics.pdf)。
実装と検証の詳細は [F303ボードのREADME](Boards/NUCLEO-F303K8/README.md) を参照してください。

### NUCLEO-L432KC

| 端子 | 機能 |
|-----|----------|
| PA8 (D9) | PWM出力（TIM1_CH1） |
| PA6 (A5) | GPIO1 |
| PA1 (A1) | PWM入力／カウンター（TIM2_CH2） |

### NUCLEO-L053R8

| 端子 | 機能 |
|-----|----------|
| PB14 | PWM出力（TIM21_CH2） |
| PA6 | GPIO1 |
| PA1 | PWM入力／カウンター（TIM2_CH2） |

### NUCLEO-L073RZ

| 端子 | 機能 |
|-----|----------|
| PB14 | PWM出力（TIM21_CH2） |
| PA6 | GPIO1 |
| PA1 | PWM入力／カウンター（TIM2_CH2） |
| PA2 | UART送信（USART2 / ST-LINK VCP） |
| PA3 | UART受信（USART2 / ST-LINK VCP） |

### STM32L0538-DISCO

| 端子 | 機能 |
|-----|----------|
| PB14 | PWM出力（TIM21_CH2） |
| PB12 | GPIO1 |
| PA7 | PWM入力／カウンター（TIM22_CH2） |
| PA9 | UART送信（USART1） |
| PA10 | UART受信（USART1） |

更新したF303K8では、PWM出力・外部カウンター・GPIO1にFT端子を使用しています。既存ボードについては、以下の端子確認結果を参照してください。5 V入力への耐性は、出力レベルが5 Vであることを意味しません。

DiscoveryボードのPA7はタッチ検出回路にも接続されています。PA7を外部カウンター入力に使用する場合は、SB25を開放してタッチ回路を切り離してください。

---

### 既存ボードの端子確認結果（2026-11-19、設定変更なし）

| ボード | PWM出力 | カウンター入力 | 実行コード上のGPIO1 | MCUの入力構造 |
|---|---|---|---|---|
| NUCLEO-L432KC | PA8 (FT) | PA1 (FT_a) | PA6 (FT_a) | 3端子とも、必要な動作条件を満たせば5 Vデジタル入力に対応 |
| NUCLEO-L053R8 | PB14 (FTf) | PA1 (FT) | PA6 (FT) | 3端子とも5 V耐性あり |
| NUCLEO-L073RZ | PB14 (FTf) | PA1 (FT) | PA6 (FT) | 3端子とも5 V耐性あり |
| STM32L0538-DISCO | PB14 (FTf) | PA7 (FT) | PA6 (FT)。資料・設定定義ではPB12 (FT) | PA6とPB12はいずれも5 V耐性あり。ただしGPIO1の割り当てに不一致あり |

この確認は、使用するMCU端子を対象としたものであり、ボード上の外部回路の5 V対応を保証するものではありません。
VDDを超える電圧を加える場合は、デジタル入力またはオルタネートファンクション入力モードを使用し、アナログ接続を無効にして、内部プル抵抗を使用しないでください。プッシュプル出力は3.3 Vです。
Discoveryの `App/Inc/board_config.h` ではGPIO1をPB12と定義していますが、`App/Inc/gpio.h` と `App/Src/gpio.c` では、既存ボードに対して現在PA6を使用しています。この不一致は報告のみで、Discoveryの設定は変更していません。PA7のカウンター入力には、前述のとおりSB25の開放も必要です。
この確認結果は、すべてのUART端子に5 V耐性があることを意味しません。L432KCのUSART2 RXに使うPA3はTT_aで、5 V耐性はありません。

出典：[L432KC DS11451：端子定義](https://www.st.com/resource/en/datasheet/stm32l432kc.pdf)、[L053R8/C8 DS10152：表15](https://www.st.com/resource/en/datasheet/stm32l053r8.pdf)、[L073RZ DS10685：表16](https://www.st.com/resource/en/datasheet/stm32l073rz.pdf)。

## ビルド設定

コンパイラーの推奨最適化設定は次のとおりです。

| 構成 | 最適化 |
|---|---|
| Debug | `-Og` |
| Release | `-Os` |

補足：

- `-Og` はコードサイズとデバッグのしやすさのバランスがよい設定です。
- STM32L053R8のFlashメモリーは64 KBです。
- STM32L073RZのFlashメモリーは192 KB、RAMは20 KBです。
- `-O0` を使用したDebugビルドでは、Flash容量を超える場合があります。

---

## 接続

### NUCLEO-F303K8

```text
D9 / PA8（PWM出力）-> D11 / PB5（カウンター入力）、SB15は変更不要
D2 / PA12 = GPIO1
```

### NUCLEO-L432KC

```text
PA8（PWM出力）-> PA1（カウンター入力）
```

### NUCLEO-L053R8

```text
PB14（PWM出力）-> PA1（カウンター入力）
```

### NUCLEO-L073RZ

```text
PB14（PWM出力）-> PA1（カウンター入力）
```

### STM32L0538-DISCO

```text
PB14（PWM出力）-> PA7（カウンター入力）
```

PA7をカウンター入力に使用する前に、SB25を開放してください。

---

## ボードの選択

`App/Inc/board_config.h` に、ボードごとに異なるタイマー・UART・GPIOの定義をまとめています。

| ボード | MCU定義 | ボード定義 | UART | カウンター |
|-------|----------------|------------------|------|---------|
| NUCLEO-L432KC | `STM32L432xx` | 不要 | USART2 (`huart2`) | TIM2 (`htim2`) |
| NUCLEO-L053R8 | `STM32L053xx` | `BOARD_NUCLEO_L053R8` | USART2 (`huart2`) | TIM2 (`htim2`) |
| NUCLEO-L073RZ | `STM32L073xx` | `BOARD_NUCLEO_L073RZ` | USART2 (`huart2`) | TIM2 (`htim2`) |
| STM32L0538-DISCO | `STM32L053xx` | `BOARD_STM32L0538_DISCO` | USART1 (`huart1`) | TIM22 (`htim22`) |

2種類のSTM32L053ボードは同じMCUファミリー定義を使用するため、UART・GPIO・カウンターのインスタンスを区別するボード定義が必要です。STM32CubeIDEのDebugとReleaseの両構成に、使用するボードのシンボルを定義してください。NUCLEO-L073RZにもボードシンボルが必要で、付属のプロジェクトでは両構成に定義済みです。

---

## libscpiのビルド方針

現在、このプロジェクトでは **libscpiをソースからビルド**しています。

libscpiのソースファイルをSTM32CubeIDEのビルド対象に含めることで、各ボードの構成に適したターゲットオプションでコンパイルします。

これにより、たとえば次のようなSTM32ターゲット間のABI不一致を避けます。

- STM32L432KC: Cortex-M4F / hard-float
- STM32L053R8: Cortex-M0+ / soft-float

静的アーカイブを使用するビルドからソースビルドに移行する際、STM32CubeMXが同等の実装を提供しているため、以下のファイルを削除しました。

```text
libscpi/Src/syscalls.c
libscpi/Src/sysmem.c
```

---

## STM32CubeMXによる再生成

STM32CubeMXでコードを再生成すると、`App` や `libscpi` などのリンクフォルダーがSTM32CubeIDEプロジェクトから削除される場合があります。

ビルド前に、`Properties > Resource > Location` が現在のリポジトリ内のボードプロジェクトを指していることを確認してください。例：

```text
X:\Tools\__Jigu-GPIO\Boards\NUCLEO_L053R8
```

異なる場所にあるリポジトリのボードプロジェクトと、別の場所の `App`・`libscpi` リンクフォルダーを混在させないでください。相対インクルードパスにより、古いヘッダーが選ばれる可能性があります。

### 復旧手順

1. STM32CubeMXでコードを再生成します。
2. Windowsエクスプローラーから、次のフォルダーをSTM32CubeIDEのProject Explorerへドラッグ＆ドロップします。

   * `App`
   * `libscpi`

3. 次の項目を選択します。

   ```text
   Link to files and folders
   ```

4. 必要に応じて、追加したファイル・フォルダーをビルド対象に戻します。

   * ファイルまたはフォルダーを右クリックします。
   * 「Resource Configurations」を選択します。
   * 「Exclude from Build...」を選択します。
   * DebugとReleaseのチェックを外します。

5. 次の操作を実行します。

   ```text
   Project -> Clean...
   Build Project
   ```

---

## 静的アーカイブのリンク

以下は、STM32CubeIDEでソースからビルドする代わりに、libscpiを静的アーカイブとしてビルドする場合の参考例です。

https://github.com/j123b567/scpi-parser/releases/tag/v2.2

### 例：NUCLEO-L432KC / Cortex-M4F hard-float

```bash
make static \
  CC=arm-none-eabi-gcc \
  AR=arm-none-eabi-ar \
  CFLAGS="-Wextra -Wmissing-prototypes -Wimplicit -Iinc \
          -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
          --specs=nano.specs -O0 -ffunction-sections -fdata-sections"
```

```bash
cp ./scpi-parser/libscpi/dist/libscpi.a \
   ./Jigu-GPIO/libscpi/dist/l432/libscpi.a
```

### 例：NUCLEO-L053R8 / Cortex-M0+ soft-float

```bash
make static \
  CC=arm-none-eabi-gcc \
  AR=arm-none-eabi-ar \
  CFLAGS="-Wextra -Wmissing-prototypes -Wimplicit -Iinc \
          -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft \
          --specs=nano.specs -O0 -ffunction-sections -fdata-sections"
```

```bash
cp ./scpi-parser/libscpi/dist/libscpi.a \
   ./Jigu-GPIO/libscpi/dist/l053/libscpi.a
```

hard-floatのCortex-M4F用アーカイブを、Cortex-M0+ターゲットに流用しないでください。
