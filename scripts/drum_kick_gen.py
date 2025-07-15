import numpy as np
from pydub import AudioSegment
import scipy.io.wavfile as wavfile
import os

sample_rate = 44100

def save_sound(filename, samples):
    samples *= 32767 / np.max(np.abs(samples))
    samples = samples.astype(np.int16)
    wavfile.write("temp.wav", sample_rate, samples)
    AudioSegment.from_wav("temp.wav").export(filename, format="mp3")

# Kick
t = np.linspace(0, 0.5, int(sample_rate * 0.5), False)
kick_freqs = np.logspace(np.log10(150), np.log10(50), t.size)
kick = np.sin(2 * np.pi * kick_freqs * t) * np.exp(-5 * t)
save_sound("kick.mp3", kick)

# Snare
t = np.linspace(0, 0.3, int(sample_rate * 0.3), False)
snare = (np.random.rand(t.size) * 2 - 1) * np.exp(-10 * t)
snare += 0.3 * np.sin(2 * np.pi * 180 * t) * np.exp(-5 * t)
save_sound("snare.mp3", snare)

# Hi-hat
t = np.linspace(0, 0.1, int(sample_rate * 0.1), False)
hihat = (np.random.rand(t.size) * 2 - 1) * np.exp(-60 * t)
save_sound("hihat.mp3", hihat)

# Tom
t = np.linspace(0, 0.4, int(sample_rate * 0.4), False)
tom = np.sin(2 * np.pi * 120 * t) * np.exp(-6 * t)
save_sound("tom.mp3", tom)

# Clap
t = np.linspace(0, 0.2, int(sample_rate * 0.2), False)
clap = np.zeros_like(t)
for delay in [0.0, 0.015, 0.03]:
    td = int(delay * sample_rate)
    clap[td:] += (np.random.rand(t.size - td) * 2 - 1) * np.exp(-30 * t[:t.size - td])
save_sound("clap.mp3", clap)

os.remove("temp.wav")
