# NUCLEO-L053R8 single-pulse hardware verification

2026-11-19, COM6, user-updated firmware; expected loopback PB14 (TIM21_CH2 PWM) to PA1 (TIM2_CH2 counter). No firmware or production source was modified for this check.

24 cases: 50 Hz and 1 kHz, requested widths 1 and 100 us, both polarities, three starts per combination. `PWM:Numbers 1`, counter reset to zero before each start.

| Polarity | Count | Cases |
|---|---:|---|
| Active High (1) | 1, stable after stop | 12/12 passed |
| Active Low (0) | 2, stable after stop | 12/12 failed the expected one-count result |

All runs stopped, and the SCPI error queue reported no error. This is a real external-input count discrepancy for Active Low; it does not establish whether the extra edge is a full pulse or a switching glitch. Pulse widths have not been measured on this board.

Original settings were restored (50 Hz, 1 us, Active High, Numbers 0, SYNC 0, counter 0), PWM was left stopped, COM6 was closed. Raw results: `hardware-single-pulse-result.json`.

## Cause investigation without firmware modification

The 64 KB target Flash matched `Boards/NUCLEO_L053R8/Release/Jigu-GPIO_L053.elf` before diagnosis and remained byte-for-byte identical afterward. Hardware breakpoints inspected the current start routine, with TIM2/TIM21 frozen while the core was halted. Debug settings, breakpoints and application settings were restored afterward.

| Stage, Active Low | PB14 / PA1 | PB14 mode | Pull | CC2E | Counter |
|---|---|---|---|---:|---:|
| Before GPIO-to-AF write | High | GPIO output | Up | 0 | 0 |
| After GPIO-to-AF, before HAL start | Low | AF | Up | 0 | 0 |
| After TIM21 output enable | High | AF | Up | 1 | 1 |
| Just before the real launch UG | High | AF | Up | 1 | 1 |
| After the requested Low pulse completes | High | idle GPIO | Up | 0 | 2 |

Thus the startup transition is High -> unwanted Low -> High (first counted rising edge), followed by the requested High -> Low -> High pulse (second counted rising edge). Pull-up remains enabled during the unwanted Low and does not prevent it. Active High starts Low and remains Low across both setup stages, so it has no extra completion edge and counts once.

The extra edge is introduced by selecting AF before TIM21 CC2E is enabled. ST RM0367 Table 104 specifies OCx=0/OCx_EN=0 when the output channel is disabled; the resulting pad level also depends on GPIO configuration. The actual pad-level transition is confirmed here, rather than inferred only from the register specification.

Source: https://www.st.com/resource/en/reference_manual/dm00095744.pdf

Raw states: `start-transition-states.json`. These breakpoint observations establish ordering and levels, not the unpaused duration of the unwanted Low. No production source or firmware correction was made.

## Common inactive-level handoff correction and regression

The subsequent common correction (2026-11-19) enables the timer output while the pin is still GPIO at the inactive level, then changes MODER to AF. Both active and preload CCR remain zero until after the handoff; the existing synchronized UG launches the requested width. No L053-specific branch, CubeMX settings, board pin configuration or board project file was changed.

On COM6, the corrected production Release passed 200 count/restart cases: both polarities, 50 Hz / 1 kHz / 10 kHz, widths 1 / 10 / 100 / 500 us where the period permits, 1 / 2 / 10 requested pulses with three repetitions, plus ten single-pulse restarts per polarity without clearing the accumulated count. One requested pulse counts once for both polarities. All completed counts remained stable and all SCPI errors were zero. Raw results: `idle-handoff-regression.json`.

A separate temporary capture firmware measured 132 pulse widths at the actual PA1 loopback input. Both polarities, 1 kHz / 10 kHz, 1 / 2 / 5 / 10 / 100 / 500 us where applicable, and the first pulse of a one-shot plus the first and last pulses of a three-shot were checked, including repeated starts. Active Low measured requested width +1 capture tick; Active High measured requested width -1 or -2 capture ticks. At the nominal 32 MHz capture clock this is +31.25 ns or -31.25/-62.5 ns. These are measured pin/capture values, not a claim of exact analog width. Raw results: `idle-handoff-width.json`.

To check that the Active High offsets were not introduced by this change, the pre-change common source (HEAD 0efe5a1) was rebuilt with the same capture instrumentation and measured in 66 matching cases. It also measured -1/-2 ticks; paired before/after differences were at most one tick (31.25 ns). No width regression was observed. Raw baseline: `idle-handoff-baseline-width.json`. Clock absolute accuracy and the physical origin of the small polarity-dependent pad/capture offset were not independently calibrated.

Hardware breakpoints on the corrected normal production firmware additionally confirmed GPIO inactive -> timer enabled with GPIO inactive -> AF inactive -> before UG inactive, with zero counter edges in all four stages for both polarities; final count was one. The debug timer-freeze settings and hardware breakpoints were restored/removed. Raw states: `idle-handoff-states.json`. These paused observations establish state/order, not the absence of an arbitrarily narrow analog spike. An oscilloscope was not available for analog waveform confirmation.

The capture firmware was removed and the corrected production Release was restored and verified. A final production-firmware check passed 10 further cases, including repeated 1/2/10/1 starts and continuous output followed by explicit stop in both polarities. It was left stopped at 50 Hz, width 1 us, Active High, Numbers 0, SYNC 0, counter 0; COM6 was closed. Raw results: `idle-handoff-final-check.json`. The local ignored Debug/Release build artifacts were updated to the tested common source. All five board variants also passed Debug and Release builds; the other three boards were not physically connected or measured.
