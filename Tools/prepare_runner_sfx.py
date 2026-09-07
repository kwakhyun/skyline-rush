"""Prepare short game cues from the preserved user WAVs (Python standard library only)."""
import array
import hashlib
import json
import math
from pathlib import Path
import wave

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'SourceArt/Audio/RunnerSFX'
# name, source excerpt start, duration, release fade, target RMS dBFS
RECIPES = [
    ('SFX_CrystalPickup', 0.0, 0.35, 0.09, -20),
    ('SFX_ObstacleHit', 0.0, 0.95, 0.12, -16),
    ('SFX_BoostActivate', 0.0, 1.40, 0.40, -18),
    ('SFX_FeverActivate', 0.25, 1.80, 0.45, -17),
]
report = []
for name, start, duration, release, target in RECIPES:
    source = ART / 'Originals' / (name + '.wav')
    with wave.open(str(source), 'rb') as wav:
        rate, channels, width, frames = wav.getframerate(), wav.getnchannels(), wav.getsampwidth(), wav.getnframes()
        assert width == 2 and channels == 2, 'Expected stereo PCM16 WAV'
        wav.setpos(round(start * rate))
        samples = array.array('h', wav.readframes(round(duration * rate)))
    frame_count = len(samples) // channels
    # A short onset ramp prevents a cut at a nonzero sample; smooth release removes long tails.
    for frame in range(frame_count):
        attack = min(1.0, frame / max(1, round(rate * 0.003)))
        tail = min(1.0, (frame_count - 1 - frame) / max(1, round(rate * release)))
        envelope = attack * (0.5 - 0.5 * math.cos(math.pi * tail))
        for channel in range(channels):
            i = frame * channels + channel
            samples[i] = round(samples[i] * envelope)
    rms = math.sqrt(sum(x*x for x in samples) / len(samples)) / 32768
    peak = max(abs(x) for x in samples) / 32768
    gain = min(10 ** (target / 20) / max(rms, 1e-9), 10 ** (-3 / 20) / max(peak, 1e-9))
    samples = array.array('h', (round(x * gain) for x in samples))
    output = ART / (name + '.wav')
    with wave.open(str(output), 'wb') as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(width)
        wav.setframerate(rate)
        wav.writeframes(samples.tobytes())
    report.append(dict(name=name, original_seconds=frames/rate, start_seconds=start,
                       seconds=frame_count/rate, fade_out_seconds=release,
                       gain_db=20*math.log10(gain), peak_dbfs=20*math.log10(max(abs(x) for x in samples)/32768),
                       original_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                       processed_sha256=hashlib.sha256(output.read_bytes()).hexdigest()))
(ART / 'processing.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
