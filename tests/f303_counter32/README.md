# F303 32-bit counter tests

Run `python tests/f303_counter32/run.py --compiler gcc` with Python and native GCC (for example Strawberry Perl's GCC). No ARM hardware is accessed.

The runner compiles the production `Boards/NUCLEO-F303K8/Core/Src/counter32.c` unchanged, with its production header and mocked TIM/HAL functions. It checks boundaries at 65535/65536, pending and serviced overflow consistency, overflow between the CNT/UIF reads, multiple wraps, modulo-2^32 behavior, arbitrary writes, stale flag removal, count retention across stop/restart, failed start, and preservation of enabled/disabled interrupt masks.

Mocks do not verify physical input timing, STM32 IRQ scheduling or CubeMX regeneration. The real F303 Debug and Release builds are checked separately.

## Common inactive-level handoff regression (2026-11-19)

The new common GPIO-to-AF handoff passed 208 production-firmware loopback count/restart cases on COM16: both polarities, 50 Hz / 1 kHz / 10 kHz, widths 1 / 10 / 100 / 500 us where valid, and requested counts 1 / 2 / 10 with repeated starts and accumulated single-shot restarts. Additional cases crossed 65536, 131072 and modulo-2^32 boundaries, and output/count agreed for 70000 pulses in each polarity. Results: `idle-handoff-regression.json`.

A temporary hardware input-capture build measured 132 widths: 1 / 2 / 5 / 10 / 100 / 500 us at 1 kHz and 1 / 10 us at 10 kHz, both polarities, the first pulse of one-shot and the first/last pulses of three-shot output, with repeated starts. Every measured width exactly matched the requested ticks at nominal 64 MHz. Results: `idle-handoff-width.json`. Absolute oscillator accuracy and analog spikes shorter than capture bandwidth were not verified.

The temporary capture firmware was removed. The normal corrected production Release was restored and verified, and 10 final checks passed (1/2/10/1 starts and continuous output/explicit stop in both polarities). Results: `idle-handoff-final-check.json`. PWM was left stopped at the prior configured 50 Hz / 100 us / Active High / Numbers 0 / SYNC 0 with counter zero, and COM16 was closed. The local ignored Debug/Release artifacts were updated to the tested source. Production changes are common across all boards, with no new board-specific start path.
