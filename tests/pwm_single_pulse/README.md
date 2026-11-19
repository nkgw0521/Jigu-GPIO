# Common PWM single-pulse integration tests

Run `python tests/pwm_single_pulse/run.py --compiler gcc` using native GCC (for example `C:\Strawberry\c\bin\gcc.exe`).

This compiles the production generator and port implementations together, using a timer/register model with CCR shadow/preload transfer and IRQ dispatch. CH1/PA8 and CH2/PB14 configurations are tested in both polarities. Cases cover exact 1/2/3/10 pulse counts, repeated single starts, first-width preload, no pulse after stop, continuous output and explicit stop, AF/GPIO mode transitions and inactive output level.

Hardware timing and electrical behavior require separate board tests. The F303 D9-to-D11 results are in `tests/f303_counter32/hardware-loopback-result.json`.

The synchronized-start tests also assert that active CCR is zero when HAL enables output, the real width starts only at the final UG, URS excludes software UGs from shot counting, blank-phase flags are cleared before the real start, a 1 us completion at IRQ unmask is not overwritten by the start routine, and failed starts restore IRQ/pin/timer state.

## Inactive-level handoff regression (2026-11-19)

The common port now keeps GPIO driving the inactive level until HAL has enabled the timer channel (and the advanced timer main output, where present). Both active and preload CCR remain zero throughout arming; only after selecting AF is the requested width written and launched with the synchronized UG. There is no board-name condition or L053-specific production path.

The host model asserts GPIO mode and inactive level at channel enable, simulates a blank-phase overflow, checks that it cannot load a real pulse early, and checks AF/channel readiness before the launch UG. Both CH1 and CH2 and both polarities pass, as does the separate counter32 suite.

All five boards' Debug and Release builds passed in isolated temporary copies; results are in `idle-handoff-builds.json`. Existing RWX LOAD segment linker warnings remain on L053, L432 and DISCO builds. Board project/configuration files were not changed.

`hardware_regression.py` runs production firmware count/restart tests over an existing PWM-to-counter loopback. Use `--extended` only for F303's software-extended counter. It leaves PWM stopped and restores this session's idle settings (50 Hz, Active High, Numbers 0, SYNC 0, counter zero; width 1 us for L053, 100 us for F303).

`hardware_width.py` uses a separately built test firmware: insert a declaration for `width_probe_arm`, call it immediately before masking interrupts in `pwm_port_start_output`, and append `width_probe.inc` to that temporary port translation unit. IC1 captures the pulse start on indirect TI2 and IC2 captures completion on direct TI2, using the counter timer's internal clock. `probe_target` selects the first or third pulse; the ISR freezes capture only after that end timestamp. An OpenOCD Tcl connection reads the volatile result symbols using addresses from the probe ELF. This probe replaces the external counter temporarily and is never part of production firmware. Restore and verify the production ELF afterward. The test scripts require Python and pyserial, and the width script also requires ARM nm and the matching ELF.

Timing records use nominal 32 MHz on L053 and 64 MHz on F303; oscillator accuracy and analog spike amplitude/duration are not calibrated. Hardware capture has finite input sampling resolution and pad/loopback delays. Full analog waveform verification still requires an oscilloscope.
