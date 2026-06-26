// Nemotron 0.6B int4 — cache-aware streaming RNN-T. A fixed 560 ms chunk
// per step (56 new mel frames + 9 pre-encode cache frames) with conformer
// caches fed forward, so per-chunk encoder cost is constant.

export const DEBUG: boolean = false; // if enabled, will calculate RTF numbers, turn to false in production

// frontend constants
export type FftSize = 32 | 64 | 128 | 256 | 512 | 1024 | 2048 | 4096 | 8192 | 16384 | 32768; // should be a power of 2
export type FftState = {
    n: number;
    bitRev: Uint16Array;
    real: Float32Array;
    imag: Float32Array;
    power: Float32Array;
    frame: Float32Array;
};

export const TARGET_SAMPLE_RATE = 16000
export const WIN_LENGTH: number = 400
export const N_FFT: number = 512
export const HOP_LENGTH: number = 160 // mel hop in samples (10 ms @ 16 kHz)
export const N_MELS: number = 128
export const PREEMPH: number = 0.97
export const LOG_ZERO_GUARD: number = 5.9604645e-8
export const OFF = (N_FFT - WIN_LENGTH) >> 1 // 56
export const PAD = N_FFT >> 1 // 256
// A streaming mel frame is stable once the right edge of the real Hann window
// is available. Currently the WIN_LENGTH window at OFF
// inside the N_FFT frame, so raw window is:
// [f * HOP_LENGTH - PAD + OFF, f * HOP_LENGTH - PAD + OFF + WIN_LENGTH)
export const STREAMING_RIGHT_CONTEXT: number = OFF + WIN_LENGTH - PAD

// model specific constants
export const NEMO_CHUNK: number = 56 // new mel frames per step (560 ms @ 10 ms hop)
export const NEMO_PRECACHE: number = 9 // pre-encode mel cache frames prepended
export const NEMO_BLANK: number = 1024
export const NEMO_VOCAB: number = 1025 // 1024 + blank
export const NEMO_MAX_SYM: number = 10
export const NEMO_FRAMES: number = NEMO_PRECACHE + NEMO_CHUNK // 65: fixed encoder input length
export const NEMO_NUM_ENCODER_LAYERS: number = 24;
export const NEMO_HIDDEN_DIM: number = 1024;
// note that this is different from left context at mel level; 
// its num of frames in the left context of encoder's attention blocks' 
export const NEMO_LEFT_CONTEXT: number = 70; 
// its num of frames in the left context of encoder's conv blocks' 
export const NEMO_CONV_CONTEXT: number = 8;
export const NEMO_DECODER_LSTM_DIM: number = 640;

// On finish, append this many chunks of silence so the cache-aware encoder
// and RNN-T decoder flush the final partial chunk — without it the trailing
// word(s) can be dropped to right-context / emission lag. The silence frames
// decode to RNN-T blanks, so the transcript is unaffected.
export const SILENCE_FLUSH_CHUNKS: number = 3

