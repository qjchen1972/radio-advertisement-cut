# -*- coding:utf-8 -*-
import time
import scipy
import librosa
import librosa.filters
import numpy as np
from scipy.io import wavfile
from pydub import AudioSegment

class Audio(object):

    def __init__(self, sample_rate, preemphasis=0.97):
        self.sample_rate = sample_rate
        self.preemphasis = preemphasis
        self.ref_level_db = 20
        self.power = 1.5
        self.gl_iters = 100
        self.num_freq = 2049
        self.frame_shift_ms = 12.5
        self.frame_length_ms = 50
        self._mel_basis = None
        self.num_mels = 80
        self.min_level_db = -100

    def load_wav(self, path):
        sr, wav = wavfile.read(path)
        #print(sr,wav.shape)
        wav = wav/np.max(np.abs(wav))
        try:
            assert sr == self.sample_rate
        except:
            print('Error:', path, 'has wrong sample rate.')
        return wav

    def save_wav(self, wav, path):
        wav *= 32767 / max(0.01, np.max(np.abs(wav)))
        wavfile.write(path, self.sample_rate, wav.astype(np.int16))

    def load_mp3(self, path, start=0, end=-1):
        mp3 = AudioSegment.from_mp3(path)
        if end == -1:
            end = mp3.duration_seconds
        mp3 = mp3[start*1000:end*1000].get_array_of_samples()
        wav = np.array(mp3.tolist()).reshape(-1,2)
        wav = wav/np.max(np.abs(wav))
        return wav	

    def _preemphasis(self, x):
        return scipy.signal.lfilter([1, -self.preemphasis], [1], x)


    def inv_preemphasis(self, x):
        return scipy.signal.lfilter([1], [1, -self.preemphasis], x)


    def spectrogram(self, y):
        D = self._stft(self._preemphasis(y))
        S = self._amp_to_db(np.abs(D)) - self.ref_level_db
        return self._normalize(S)


    def inv_spectrogram(self, spectrogram):
        '''Converts spectrogram to waveform using librosa'''
        S = self._db_to_amp(self._denormalize(spectrogram) + self.ref_level_db)# Convert back to linear
        return self.inv_preemphasis(self._griffin_lim(S ** self.power))# Reconstruct phase

    def melspectrogram(self, y):
        D = self._stft(self._preemphasis(y))
        S = self._amp_to_db(self._linear_to_mel(np.abs(D))) - self.ref_level_db
        return self._normalize(S)

    def inv_melspectrogram(self, spectrogram):
        mel = _db_to_amp(_denormalize(spectrogram) + self.ref_level_db)
        S = _mel_to_linear(mel)
        return inv_preemphasis(_griffin_lim(S ** self.power))

    def find_endpoint(self, wav, threshold_db=-40, min_silence_sec=0.8):
        window_length = int(self.sample_rate * min_silence_sec)
        hop_length = int(window_length / 4)
        threshold = _db_to_amp(threshold_db)
        for x in range(hop_length, len(wav) - window_length, hop_length):
            if np.max(wav[x:x+window_length]) < threshold:
                return x + hop_length
        return len(wav)

    def _griffin_lim(self, S):
        '''librosa implementation of Griffin-Lim
        Based on https://github.com/librosa/librosa/issues/434
        '''
        angles = np.exp(2j * np.pi * np.random.rand(*S.shape))
        S_complex = np.abs(S).astype(np.complex)
        y = _istft(S_complex * angles)
        for i in range(self.gl_iters):
            angles = np.exp(1j * np.angle(_stft(y)))
            y = _istft(S_complex * angles)
        return y


    def _stft(self, y):
        n_fft, hop_length, win_length = self._stft_parameters()
        #print('stft',n_fft, hop_length, win_length)
        return librosa.stft(y=y, n_fft=n_fft, hop_length=hop_length, win_length=win_length)


    def _istft(self, y):
        _, hop_length, win_length = self._stft_parameters()
        return librosa.istft(y, hop_length=hop_length, win_length=win_length)


    def _stft_parameters(self):
        n_fft = (self.num_freq - 1) * 2
        hop_length = int(self.frame_shift_ms / 1000 * self.sample_rate)
        win_length = int(self.frame_length_ms / 1000 * self.sample_rate)
        return n_fft, hop_length, win_length   

    def _linear_to_mel(self, spectrogram):
        if self._mel_basis is None:
            self._mel_basis = self._build_mel_basis()
        return np.dot(self._mel_basis, spectrogram)


    def _mel_to_linear(self, spectrogram):
        if self._mel_basis is None:
            self._mel_basis = _build_mel_basis()
        inv_mel_basis = np.linalg.pinv(self._mel_basis)
        inverse = np.dot(inv_mel_basis, spectrogram)
        inverse = np.maximum(1e-10, inverse)
        return inverse


    def _build_mel_basis(self):
        n_fft = (self.num_freq - 1) * 2
        return librosa.filters.mel(self.sample_rate, n_fft, n_mels=self.num_mels)

    def _amp_to_db(self, x):
        return 20 * np.log10(np.maximum(1e-5, x))

    def _db_to_amp(self, x):
        return np.power(10.0, x * 0.05)

    def _normalize(self, S):
        return np.clip((S - self.min_level_db) / -self.min_level_db, 0, 1)

    def _denormalize(self, S):
        return (np.clip(S, 0, 1) * - self.min_level_db) + self.min_level_db

    def get_mel(self, filename):
        wav = self.load_wav(filename)
        mel = self.melspectrogram(wav).astype(np.float32)
        return mel


class Testaudio(object):
   
    def __init__(self, sample_rate, preemphasis=0.97):
        self.sr = sample_rate
        self.preemphasis = preemphasis
        self.ref_db = 20
        self.power = 1.5
        self.n_iters = 50
        self.n_fft = 2048
        self.frame_shift = 0.0125
        self.frame_length = 0.05
        self.hop_length = int(self.sr * self.frame_shift)
        self.win_length = int(self.sr * self.frame_length)
        self._mel_basis = None
        self.n_mels = 80
        self.max_db = 100
        self.r = 1

    def get_spectrograms(self, fpath, use_path=True):
        '''Returns normalized log(melspectrogram) and log(magnitude) from `sound_file`.
           Args:
               sound_file: A string. The full path of a sound file.
           Returns:
               mel: A 2d array of shape (T, n_mels) <- Transposed
               mag: A 2d array of shape (T, 1+n_fft/2) <- Transposed
        '''

        # Loading sound file
        if use_path:
            #y, sr = librosa.load(fpath, sr=self.sr)
            sr, y = wavfile.read(fpath)
        else:
            y, sr = fpath, hp.sr
        print("y.shape: ", y.shape, y)
        print("sr: ", sr)

        time1 = time.time()
        # Preemphasis pre-emphasis，预加重
        y = np.append(y[0], y[1:] - self.preemphasis * y[:-1])
        print(y.shape,y)

        # stftz
        linear = librosa.stft(y=y,
                              n_fft=self.n_fft,
                              hop_length=self.hop_length,
                              win_length=self.win_length)
        print('stf',linear)
        
        # magnitude spectrogram
        mag = np.abs(linear)  # (1+n_fft//2, T)
        # mel spectrogram
        mel_basis = librosa.filters.mel(self.sr, self.n_fft, self.n_mels)  # (n_mels, 1+n_fft//2)
        print('basis', mel_basis)

        mel = np.dot(mel_basis, mag)  # (n_mels, t)
        
        print('dot',mel)

        # to decibel
        mel = 20 * np.log10(np.maximum(1e-5, mel))
        mag = 20 * np.log10(np.maximum(1e-5, mag))

        # normalize
        mel = np.clip((mel - self.ref_db + self.max_db) / self.max_db, 1e-8, 1)
        mag = np.clip((mag - self.ref_db + self.max_db) / self.max_db, 1e-8, 1)
        
        print('norm', mel.shape)

        # Transpose
        mel = mel.T.astype(np.float32)  # (T, n_mels)
        mag = mag.T.astype(np.float32)  # (T, 1+n_fft//2)

        #
        mel = mel[:len(mel) // self.r * self.r].reshape([len(mel) // self.r, self.r * self.n_mels])
        mag = mag[:len(mag) // self.r * self.r]  # .reshape([len(mag)//hp.r,hp.r*1025])

        time2 = time.time()
        print("cost time:", time2-time1)
        print('ans', mel.shape)

        return mel, mag


    # pcen-mel特征提取
    def get_pcen(self, fpath, use_path=True):
        # Loading sound file
        if use_path:
            y, sr = librosa.load(fpath, sr=self.sr)
        else:
            y, sr = fpath, self.sr
        S = librosa.feature.melspectrogram(y, sr=sr, power=1, n_fft=self.n_fft, hop_length=self.hop_length, n_mels=self.n_mels)
        pcen_S = librosa.pcen(S).T
        log_S = librosa.amplitude_to_db(S, ref=np.max)
        return pcen_S  # ,log_S


if __name__ == '__main__':
    
    testaudio = Testaudio(16000)
    mel, mag = testaudio.get_spectrograms("D12_752.wav")
    
    audio = Audio(16000)
    audio.get_mel("D12_752.wav")
    