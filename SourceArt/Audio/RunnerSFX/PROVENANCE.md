# User-supplied runner sound effects

Four generated WAVs were supplied by the user on 2026-09-07 for SKYLINE RUSH: crystal pickup (1), obstacle hit (6), boost activation (7), and fever activation (8).

Originals/ preserves their exact bytes under stable asset names. processing.json records the original and prepared SHA-256 hashes, excerpt boundaries, fades and gain. Tools/prepare_runner_sfx.py reproducibly creates separate stereo PCM16 / 48 kHz game versions. No original recording is overwritten.

| Event | Source length | Prepared length | Treatment |
|---|---:|---:|---|
| Crystal pickup | 4.96 s | 0.35 s | Keep initial transient; shorten repeating tail |
| Obstacle hit | 2.00 s | 0.95 s | Keep impact and decay; remove near-silent ending |
| Boost activation | 5.00 s | 1.40 s | Keep onset and rising sweep; smooth release |
| Fever activation | 8.76 s | 1.80 s | Start at 0.25 s; keep one initial burst with smooth release |

All prepared clips have a 3 ms onset ramp and peak ceiling of -3 dBFS. RMS gain targets are bounded by this ceiling; they are not LUFS measurements. Game event gains are applied separately. Public source exports include the recipe and metadata, not the raw or prepared recordings. This source record does not grant a new license.
