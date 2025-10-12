# Jigu-GPIO

## link archive

https://github.com/j123b567/scpi-parser/releases/tag/v2.2

## SCPI Commands

| Commands | Value | Description |
|----------|-------|-------------|
| SYSTem:LED | 0: Off / 1: On | - |
| GPIO:SELect | 1 | - |
| GPIO:SELect? | - | - |
| GPIO:DIRection  | IN/OUTPushpull/OUTOpendrain |
| GPIO:DIRection? | - | - |
| GPIO:LEVel | 0: Lo / 1: Hi | - |
| GPIO:LEVel? | - | - |
| GPIO:PULl | No/Down/Up | - |
| GPIO:PULl? | - | - |
| GPIO:Speed | Low/Medium/High/Veryhigh | - |
| GPIO:Speed? | - | - |
| PWM:Frequency | 0 - 4294967295 | - |
| PWM:Frequency? | - | - |
| PWM:Width | 0 - 4294967295 | - |
| PWM:Width? | - | - |
| PWM:POLarity | 0: - / 1: + | - |
| PWM:POLarity? | - | - |
| PWM:STart | 0:Stop / 1:Start | - |
| PWM:STart? | - | - |
| PWM:Count | 0 - 4294967295 | - |
| PWM:Count? | - | - |

## Haredware

[NUCLEO-L432KC](https://os.mbed.com/platforms/ST-Nucleo-L432KC/)

| Pin | Function |
|-----|----------|
| PA5(A4) | PWM Out |
| PA6(A5) | GPIO1 |
| PA8(D9) | PWM In(Countor) |