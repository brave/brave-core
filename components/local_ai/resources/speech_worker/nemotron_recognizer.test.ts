// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// These unit tests use the real NemotronStreamSession and streaming frontend,
// including audio buffering, chunking, processAvailable(), state propagation, and
// RNN-T decoding. Only the model inference boundary is mocked:
// runEncoder() and runDecoder() return synthetic encoder/decoder outputs instead
// of invoking ONNX Runtime and the actual Nemotron model.

import { describe, expect, it, jest } from '@jest/globals'

import { NemotronStreamSession, OrtNemotronModel } from './nemotron_recognizer'

import * as config from './configs'

jest.mock('./ort_env', () => ({
  // Used by processAvailable(); no-op for unit tests
  disposeOrt: jest.fn(),

  // Not used by these tests, but required by OrtNemotronModel.buildFromBytes().
  ensureOrt: jest.fn(),
}))

type TensorData = Float32Array | Int32Array | BigInt64Array

class TestTensor {
  readonly type: string
  readonly data: TensorData
  readonly dims: readonly number[]

  constructor(type: string, data: TensorData, dims: readonly number[]) {
    this.type = type
    this.data = data
    this.dims = dims
  }

  // for structural closeness to the real OrtTensor
  dispose(): void {}
}

// inputs to runEncoder()
type EncoderFeeds = {
  audio_signal: TestTensor
  length: TestTensor
  cache_last_channel: TestTensor
  cache_last_time: TestTensor
  cache_last_channel_len: TestTensor
}

// inputs to runDecoder()
type DecoderFeeds = {
  encoder_outputs: TestTensor
  targets: TestTensor
  target_length: TestTensor
  input_states_1: TestTensor
  input_states_2: TestTensor
}

// similar to onResult property of NemotronStreamSession
type Result = {
  text: string
  isFinal: boolean
}

// Exposed as OrtNemotronModel to the session, but encoder and decoder
// inference are replaced with Jest mocks.
type MockModel = {
  model: OrtNemotronModel
  runEncoder: jest.Mock
  runDecoder: jest.Mock
}

// Number of raw PCM samples needed for `frameCount` stable mel frames.
// stableFrames = floor((sampleCount - STREAMING_RIGHT_CONTEXT) / HOP_LENGTH) + 1
function samplesForStableFrames(frameCount: number): number {
  if (frameCount <= 0) {
    return 0
  }
  return config.STREAMING_RIGHT_CONTEXT + (frameCount - 1) * config.HOP_LENGTH
}

// Raw samples needed before exactly `chunkCount` complete encoder chunks can be available.
function samplesForChunks(chunkCount: number): number {
  return samplesForStableFrames(chunkCount * config.NEMO_CHUNK)
}

// Once the first chunk is available, each additional encoder chunk advances by exactly 56 * 160 samples.
const CHUNK_ADVANCE_SAMPLES = config.NEMO_CHUNK * config.HOP_LENGTH

// pause until the session is no longer busy
async function waitForIdle(session: NemotronStreamSession): Promise<void> {
  await (
    session as unknown as {
      inflight: Promise<void>
    }
  ).inflight
}

// create mock/fake encoder results
function makeEncoderResult(cacheMarker = 0, cacheLen = 0, nEnc = 1) {
  const nTime = 1

  return {
    outputs: new TestTensor(
      'float32',
      new Float32Array(config.NEMO_HIDDEN_DIM * nTime),
      [1, config.NEMO_HIDDEN_DIM, nTime],
    ),

    encoded_lengths: new TestTensor(
      'int64',
      BigInt64Array.from([BigInt(nEnc)]),
      [1],
    ),

    cache_last_channel_next: new TestTensor(
      'float32',
      Float32Array.from([cacheMarker]),
      [1],
    ),

    cache_last_time_next: new TestTensor(
      'float32',
      Float32Array.from([cacheMarker]),
      [1],
    ),

    cache_last_channel_next_len: new TestTensor(
      'int64',
      BigInt64Array.from([BigInt(cacheLen)]),
      [1],
    ),
  }
}

// create mock/fake decoder results
function makeDecoderResult(token: number, stateMarker = 0) {
  const logits = new Float32Array(config.NEMO_VOCAB)

  logits[token] = 1

  return {
    outputs: new TestTensor('float32', logits, [1, config.NEMO_VOCAB]),

    output_states_1: new TestTensor(
      'float32',
      Float32Array.from([stateMarker]),
      [1],
    ),

    output_states_2: new TestTensor(
      'float32',
      Float32Array.from([stateMarker]),
      [1],
    ),
  }
}

// Creates a mock model for NemotronStreamSession with simulated encoder and
// decoder inference.
function createMockModel(): MockModel {
  const tokens: string[] = []

  tokens[1] = '▁hello'
  tokens[2] = '▁world'
  tokens[config.NEMO_BLANK] = '<blank>'

  const runEncoder = jest.fn(async () => makeEncoderResult())

  const runDecoder = jest.fn(async () => makeDecoderResult(config.NEMO_BLANK))

  const ort = {
    Tensor: TestTensor,
  }

  const fbank = new Float32Array(config.N_MELS * (config.N_FFT / 2 + 1))

  const hann = new Float32Array(config.WIN_LENGTH)

  hann.fill(1)

  const model = {
    ort,
    tokens,
    fbank,
    hann,
    runEncoder,
    runDecoder,
  } as unknown as OrtNemotronModel // just to satisfy the type checker

  return {
    model,
    runEncoder,
    runDecoder,
  }
}

//
function createSession(mock: MockModel) {
  const results: Result[] = []

  const onResult = jest.fn((text: string, isFinal: boolean) => {
    results.push({
      text,
      isFinal,
    })
  })

  const onError = jest.fn()

  const session = new NemotronStreamSession(
    mock.model,
    config.TARGET_SAMPLE_RATE,
    onResult,
    onError,
  )

  return {
    session,
    results,
    onResult,
    onError,
  }
}

describe('NemotronStreamSession', () => {
  describe('construction', () => {
    // Streaming tests for session construction verifying that -
    // (a) given a valid sample rate, a session can be successfully created
    // (b) invalid sample rate shouldnt create session

    it('accepts the Nemotron sample rate', () => {
      const mock = createMockModel()

      expect(
        () =>
          new NemotronStreamSession(
            mock.model,
            config.TARGET_SAMPLE_RATE,
            () => {},
            () => {},
          ),
      ).not.toThrow()
    })

    it('rejects unsupported sample rates', () => {
      const mock = createMockModel()
      const wrongSamplingRate = 8000

      expect(
        () =>
          new NemotronStreamSession(
            mock.model,
            wrongSamplingRate,
            () => {},
            () => {},
          ),
      ).toThrow('Unsupported sample rate for Nemotron: 8000 Hz')
    })
  })

  describe('audio buffering', () => {
    // Streaming tests for buffering verifying that -
    // (a) given less than one full encoder chunk, encoder or decoder are not run
    // (b) given one full encoder chunk only, run encoder and decoder only once
    // (c) given N full encoder chunks, run encoder and decoder N times
    // (d) given one full chunk and a half chunk, run encoder only once.
    // Then add a new half chunk and the encoder should run only once more.
    // (e) how many encoder chunks eventually get processed is independent
    // of how the browser chunks the raw audio into addAudio() calls.
    it('does not infer before one full encoder chunk is available', async () => {
      const mock = createMockModel()
      const { session } = createSession(mock)

      session.addAudio(new Float32Array(samplesForChunks(1) - 1))

      await waitForIdle(session)

      expect(mock.runEncoder).not.toHaveBeenCalled()

      expect(mock.runDecoder).not.toHaveBeenCalled()
    })

    it('processes exactly one encoder chunk once enough raw audio is available', async () => {
      const mock = createMockModel()
      const { session } = createSession(mock)
      const N = 1

      session.addAudio(new Float32Array(samplesForChunks(N)))

      await waitForIdle(session)

      expect(mock.runEncoder).toHaveBeenCalledTimes(N)

      // One encoded frame (because nEnc=1), decoder immediately returns blank.
      expect(mock.runDecoder).toHaveBeenCalledTimes(1)
    })

    it('drains all complete chunks', async () => {
      const mock = createMockModel()
      const { session } = createSession(mock)
      const N = 3

      session.addAudio(new Float32Array(samplesForChunks(N)))

      await waitForIdle(session)

      expect(mock.runEncoder).toHaveBeenCalledTimes(N)

      // here, num of decoder runs is also N, only because nEnc=1.
      // if nEnc is changed in makeEncoderResult, then num of decoder calls would change
      expect(mock.runDecoder).toHaveBeenCalledTimes(N)
    })

    it('retains incomplete audio for the next addAudio call', async () => {
      const mock = createMockModel()
      const { session } = createSession(mock)

      const firstChunk = samplesForChunks(1)

      const halfAdvance = CHUNK_ADVANCE_SAMPLES / 2

      session.addAudio(new Float32Array(firstChunk + halfAdvance))

      await waitForIdle(session)

      expect(mock.runEncoder).toHaveBeenCalledTimes(1)

      session.addAudio(new Float32Array(halfAdvance))

      await waitForIdle(session)

      // twice in total
      expect(mock.runEncoder).toHaveBeenCalledTimes(2)
    })

    it('is independent of incoming audio packet boundaries', async () => {
      const total = samplesForChunks(2)

      const wholeMock = createMockModel()
      const { session: wholeSession } = createSession(wholeMock)

      wholeSession.addAudio(new Float32Array(total))

      await waitForIdle(wholeSession)

      const splitMock = createMockModel()
      const { session: splitSession } = createSession(splitMock)

      const packetSizes = [
        3000,
        3000,
        3000, // cumulative 9000 -> first encoder chunk becomes available
        4480,
        total - 13480,
      ]

      for (const size of packetSizes) {
        splitSession.addAudio(new Float32Array(size))
        await waitForIdle(splitSession)
      }

      expect(wholeMock.runEncoder).toHaveBeenCalledTimes(2)

      expect(splitMock.runEncoder).toHaveBeenCalledTimes(2)
    })
  })

  describe('encoder state', () => {
    // Streaming tests for encoder state verifying that -
    // (a) the previous encoder call’s cache is fed into the next call
    it('passes encoder cache outputs into the next encoder call', async () => {
      const mock = createMockModel()

      const firstCacheMarker = 42
      const firstCacheLen = 1

      const { session } = createSession(mock)

      mock.runEncoder
        .mockImplementationOnce(async () =>
          makeEncoderResult(firstCacheMarker, firstCacheLen),
        )
        .mockImplementationOnce(async () => makeEncoderResult(84, 2))

      session.addAudio(new Float32Array(samplesForChunks(2)))

      await waitForIdle(session)

      expect(mock.runEncoder).toHaveBeenCalledTimes(2)

      const secondFeeds = mock.runEncoder.mock.calls[1][0] as EncoderFeeds

      expect((secondFeeds.cache_last_channel.data as Float32Array)[0]).toBe(
        firstCacheMarker,
      )

      expect((secondFeeds.cache_last_time.data as Float32Array)[0]).toBe(
        firstCacheMarker,
      )

      expect(
        (secondFeeds.cache_last_channel_len.data as BigInt64Array)[0],
      ).toBe(BigInt(firstCacheLen))
    })
  })

  describe('RNN-T decoding', () => {
    // Streaming tests for RNN-T decoding verifying that -
    // (a) the decoder stops decoding the current time frame when it emits a blank token
    // (b) when the decoder emits a real token instead of blank,
    // that token is appended to hyp, converted to text, and emitted as an interim result.
    // (c) the transcript hypothesis is persistent across encoder chunks
    // rather than being reset after every chunk.
    // (d) even if blank is emitted in-between, previous non-blank token and LSTM states are
    // provided to later decoder calls
    // (e) NEMO_MAX_SYM is the max num of continuous non blank tokens produced by an encoder time frame
    // (f) if the decoder processes a chunk but emits only blanks,
    // the session should not send an interim transcript callback.
    it('stops decoding a frame when blank is emitted', async () => {
      const mock = createMockModel()

      const { session } = createSession(mock)

      mock.runDecoder.mockResolvedValue(makeDecoderResult(config.NEMO_BLANK))

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(mock.runDecoder).toHaveBeenCalledTimes(1)
    })

    it('adds non-blank tokens to the hypothesis', async () => {
      const mock = createMockModel()
      const fakeLSTMStateMarker = 10

      const { session, results } = createSession(mock)

      let call = 0

      mock.runDecoder.mockImplementation(async () => {
        if (call++ === 0) {
          return makeDecoderResult(1, fakeLSTMStateMarker)
        }

        return makeDecoderResult(config.NEMO_BLANK)
      })

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(results).toEqual([
        {
          text: 'hello',
          isFinal: false,
        },
      ])
    })

    it('accumulates tokens across encoder chunks', async () => {
      const mock = createMockModel()

      // can be any numbers, not really relevant for this test case
      const fakeLSTMStateMarker1 = 10
      const fakeLSTMStateMarker2 = 20

      const { session, results } = createSession(mock)

      let call = 0

      mock.runDecoder.mockImplementation(async () => {
        switch (call++) {
          case 0:
            return makeDecoderResult(1, fakeLSTMStateMarker1)

          case 1:
            return makeDecoderResult(config.NEMO_BLANK)

          case 2:
            return makeDecoderResult(2, fakeLSTMStateMarker2)

          default:
            return makeDecoderResult(config.NEMO_BLANK)
        }
      })

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      session.addAudio(new Float32Array(CHUNK_ADVANCE_SAMPLES))

      await waitForIdle(session)

      expect(results).toEqual([
        {
          text: 'hello',
          isFinal: false,
        },
        {
          text: 'hello world',
          isFinal: false,
        },
      ])
    })

    it('passes the previous token and LSTM state to later decoder calls', async () => {
      const mock = createMockModel()
      const fakeLSTMStateMarker = 123

      const { session } = createSession(mock)

      let call = 0

      mock.runDecoder.mockImplementation(async () => {
        if (call++ === 0) {
          return makeDecoderResult(1, fakeLSTMStateMarker)
        }

        return makeDecoderResult(config.NEMO_BLANK)
      })

      session.addAudio(new Float32Array(samplesForChunks(2)))

      await waitForIdle(session)

      // decoder call 0: token 1; call 1: blank for same encoder frame
      // call 2: next encoder chunk
      expect(mock.runDecoder).toHaveBeenCalledTimes(3)

      const secondChunkFeeds = mock.runDecoder.mock.calls[2][0] as DecoderFeeds

      expect((secondChunkFeeds.targets.data as Int32Array)[0]).toBe(1)

      expect((secondChunkFeeds.input_states_1.data as Float32Array)[0]).toBe(
        fakeLSTMStateMarker,
      )

      expect((secondChunkFeeds.input_states_2.data as Float32Array)[0]).toBe(
        fakeLSTMStateMarker,
      )
    })

    it('limits emissions to NEMO_MAX_SYM for one encoder frame', async () => {
      const mock = createMockModel()

      const { session } = createSession(mock)

      // Never emit blank.
      mock.runDecoder.mockResolvedValue(makeDecoderResult(1))

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(mock.runDecoder).toHaveBeenCalledTimes(config.NEMO_MAX_SYM)
    })

    it('does not emit an empty interim result', async () => {
      const mock = createMockModel()

      const { session, onResult } = createSession(mock)

      mock.runDecoder.mockResolvedValue(makeDecoderResult(config.NEMO_BLANK))

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(onResult).not.toHaveBeenCalled()
    })
  })

  describe('finish', () => {
    // Streaming tests for session finish verifying that -
    // (a) finish() does not silently end without producing a single final callback.
    // (b) the contents of that single callback when nothing was recognized are specifically { text: '', isFinal: true }.
    // (c) calling finish() multiple times has the same effect as calling it once.
    // (d) finish() closes the stream to new audio immediately, even if the async final flush is still running.
    it('emits one final result', async () => {
      const mock = createMockModel()

      const { session, onResult } = createSession(mock)

      session.finish()

      await waitForIdle(session)

      const finalCalls = onResult.mock.calls.filter(([, isFinal]) => isFinal)

      expect(finalCalls).toHaveLength(1)
    })

    it('can emit an empty final result', async () => {
      const mock = createMockModel()

      const { session, results } = createSession(mock)

      session.finish()

      await waitForIdle(session)

      expect(results.at(-1)).toEqual({
        text: '',
        isFinal: true,
      })
    })

    it('is idempotent', async () => {
      const mock = createMockModel()

      const { session, onResult } = createSession(mock)

      session.finish()
      session.finish()
      session.finish()

      await waitForIdle(session)

      const finalCalls = onResult.mock.calls.filter(([, isFinal]) => isFinal)

      expect(finalCalls).toHaveLength(1)
    })

    it('ignores audio after finish begins', async () => {
      const baselineMock = createMockModel()
      const { session: baselineSession } = createSession(baselineMock)

      baselineSession.finish()
      await waitForIdle(baselineSession)

      const baselineCalls = baselineMock.runEncoder.mock.calls.length

      const mock = createMockModel()
      const { session } = createSession(mock)

      session.finish()

      // This should be ignored because state is already "finishing".
      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(mock.runEncoder).toHaveBeenCalledTimes(baselineCalls)
    })
  })

  describe('error handling', () => {
    // Streaming tests for handling errors verifying that -
    // (a) encoder inference failure should terminate recognition rather than leave the pipeline hanging or continue processing subsequent audio.
    // (b) same as (a) for decoder inference failure
    // (c) inference failure results in session state going to 'ended'
    // (d) onError is called only once even if finish gets called after an inference fail
    it('calls onError when encoder inference fails', async () => {
      const mock = createMockModel()

      const { session, onError, onResult } = createSession(mock)

      const consoleSpy = jest
        .spyOn(console, 'error')
        .mockImplementation(() => {})

      mock.runEncoder.mockRejectedValue(new Error('encoder failed'))

      session.addAudio(new Float32Array(samplesForChunks(1)))

      await waitForIdle(session)

      expect(onError).toHaveBeenCalledTimes(1)

      expect(onResult).not.toHaveBeenCalled()

      consoleSpy.mockRestore()
    })

    it('calls onError when decoder inference fails', async () => {
      const mock = createMockModel()

      const { session, onError, onResult } = createSession(mock)

      const consoleSpy = jest
        .spyOn(console, 'error')
        .mockImplementation(() => {})

      try {
        mock.runDecoder.mockRejectedValue(new Error('decoder failed'))

        session.addAudio(new Float32Array(samplesForChunks(1)))

        await waitForIdle(session)

        expect(mock.runEncoder).toHaveBeenCalledTimes(1)
        expect(mock.runDecoder).toHaveBeenCalledTimes(1)

        expect(onError).toHaveBeenCalledTimes(1)
        expect(onResult).not.toHaveBeenCalled()
      } finally {
        consoleSpy.mockRestore()
      }
    })

    it('enters a terminal state after inference failure', async () => {
      const mock = createMockModel()
      const { session, onError } = createSession(mock)

      const consoleSpy = jest
        .spyOn(console, 'error')
        .mockImplementation(() => {})

      try {
        mock.runEncoder.mockRejectedValueOnce(new Error('encoder failed'))

        session.addAudio(new Float32Array(samplesForChunks(1)))

        await waitForIdle(session)

        expect(mock.runEncoder).toHaveBeenCalledTimes(1)
        expect(onError).toHaveBeenCalledTimes(1)

        // Stream has entered the terminal 'ended' state,
        // so subsequent audio should be ignored.
        session.addAudio(new Float32Array(samplesForChunks(1)))

        await waitForIdle(session)

        expect(mock.runEncoder).toHaveBeenCalledTimes(1)
        expect(onError).toHaveBeenCalledTimes(1)
      } finally {
        consoleSpy.mockRestore()
      }
    })

    it('calls onError only once even if finish is called after failure', async () => {
      const mock = createMockModel()
      const { session, onError } = createSession(mock)

      const consoleSpy = jest
        .spyOn(console, 'error')
        .mockImplementation(() => {})

      try {
        mock.runEncoder.mockRejectedValueOnce(new Error('encoder failed'))

        session.addAudio(new Float32Array(samplesForChunks(1)))

        await waitForIdle(session)

        expect(mock.runEncoder).toHaveBeenCalledTimes(1)
        expect(onError).toHaveBeenCalledTimes(1)

        session.finish()

        await waitForIdle(session)

        expect(mock.runEncoder).toHaveBeenCalledTimes(1)
        expect(onError).toHaveBeenCalledTimes(1)
      } finally {
        consoleSpy.mockRestore()
      }
    })
  })
})
