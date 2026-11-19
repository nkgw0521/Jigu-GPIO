# NUCLEO-L073RZ

NUCLEO-L073RZ用の現行CubeMX/CubeIDEプロジェクトです。旧NUCLEO_L073RZ参照用プロジェクトは削除しました。

## アプリケーションの統合

- Appとlibscpiは、PARENT-2-PROJECT_LOCを使用して../../Appと../../libscpiにリンクしています。
- DebugとReleaseの両方で、STM32L073xxとBOARD_NUCLEO_L073RZを定義して共通ソースをコンパイルします。
- Debugは-Og、Releaseは-Osを使用します。
- main.cはGPIO、DMA、TIM2、TIM21、USART2の初期化後にapp_main()を呼び出します。
- USART2 RXは循環DMA1_Channel5を使用し、UART・DMA・タイマーの割り込みハンドラーを組み込んでいます。
- リンカースクリプトにはプロジェクト相対パスを使用し、UNC共有からもビルドできるようにしています。

## ハードウェア

| 信号 | 端子・ペリフェラル |
| --- | --- |
| PWM出力 | PB14 / TIM21_CH2 |
| 外部カウンター | PA1 / TIM2_CH2、外部クロックモード1、TI2FP2 |
| GPIO1 | PA6 |
| ST-LINK仮想COM | PA2 TX / PA3 RX、USART2 |
| オンボードLED | PA5 |
| デバッグ | PA13 / PA14、SWD |

クロックはST-LINKからのHSEバイパス8 MHz、PLL ×8 /2で、SYSCLKとタイマークロックは32 MHzです。
TIM21の初期設定はPSC=31、ARR=999、CCR2=500（1 kHz、500 us）です。
起動後のPWM設定は共通アプリケーションで制御します。

## SCPIの検証

ST-LINKのUSBコネクターと仮想COMポートを使用してください。通信条件は115200 baud、データ8ビット、パリティなし、ストップ1ビット、フロー制御なしです。
コマンドの終端はLFです。共通アプリケーションは入力をエコーバックします。

ビルドしたELFを書き込んでボードをリセットした後、次のコマンドを実行します。

    Copy-Item .\Test-Scpi.ps1 "$env:TEMP\Jigu-Test-Scpi.ps1"
    powershell -NoProfile -File "$env:TEMP\Jigu-Test-Scpi.ps1" -PortName COM14

COM14は、現在のST-LINKのCOMポートに置き換えてください。スクリプトをローカルにコピーすることで、RemoteSignedポリシー下でUNC共有上のスクリプトがリモートスクリプトとして扱われることも避けられます。

このスクリプトは、識別情報、操作完了、GPIO選択、PWM設定の問い合わせ、SCPIエラーキューを確認します。
PWMの開始やGPIO出力の変更は行いません。既存のエラーキューから1件を読み取ります。
共通の識別文字列は、現在MANUFACTURE,INSTR2013で始まります。

外部パルスカウントを検証する場合は、PB14とPA1を接続し、プロジェクトのPWMコマンドを使用してください。

CubeMXでコードを再生成した後は、App/libscpiのリンクリソース、ソース設定、インクルードパス、BOARD_NUCLEO_L073RZ、app_main()の呼び出しが残っていることを確認してください。

## 検証状況（2026-11-17）

- DebugとReleaseのビルド：成功。エラー・警告ともに0件です。
- Flash使用量（text + data）：Debugは54,368バイト、Releaseは49,616バイトです。
- 静的データ + BSS：3,816バイトです。実行時のスタック・ヒープ使用量は含みません。
- COM14 / ST-LINK 066FFF554857707067024551に対応するボードへDebug ELFを書き込み、Flash検証に成功し、MCUを再起動しました。
- 搭載されているST-LINK V2J25M13はCubeProgrammerに拒否されました。同梱のOpenOCD、interface/stlink-dap.cfg、dapdirect_swd、target/stm32l0x.cfgを使用して書き込みに成功しました。ST-LINKのファームウェアは更新していません。
- MCUが共通のUART受信ループに到達し、UART2 RXの循環DMAが初期化されることを確認しています。
- シリアル経由のSCPI応答の一連の動作は、まだ確認していません。

ユーザーにより、SB13とSB14が切断されていることが確認されています。この状態では、オンボードST-LINKのVCPとターゲットのUSART2が切り離されています。
診断中には外部UARTのループバックも見つかり、取り外しました。
COM14で検証を続ける場合は、SB13/SB14を再接続し、SB62/SB63は切断したまま、Test-Scpi.ps1を再実行してください。
MCUは新しいDebugファームウェアで動作したままにしています。

ボードのUART配線の参考資料：ST UM1724の7.10節とはんだブリッジ表。
https://www.st.com/resource/en/user_manual/um1724-stlinkv21-in-circuit-debuggerprogrammer-for-stm8-and-stm32-stmicroelectronics.pdf
