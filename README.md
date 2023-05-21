# Jigu-GPIO

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
| PWM:FREQuency | 0 - 4294967295 | - |
| PWM:FREQuency? | - | - |
| PWM:Width | 0 - 4294967295 | - |
| PWM:Width? | - | - |
| PWM:POLarity | 0: - / 1: + | - |
| PWM:POLarity? | - | - |

## Haredware

[NUCLEO-L432KC](https://os.mbed.com/platforms/ST-Nucleo-L432KC/)

| Pin | Function |
|-----|----------|
| PA5(A4) | PWM Out |
| PA6(A5) | GPIO1 |
