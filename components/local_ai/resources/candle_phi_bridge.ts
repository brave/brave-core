// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CandleService,
  PhiInterfaceReceiver,
  LargeModelFiles,
} from 'gen/brave/components/local_ai/common/candle.mojom.m.js'

console.log('[Candle WASM] Phi Script loaded')

// Import the WASM Phi Model class from candle_phi
// @ts-expect-error - Module path will be resolved at runtime via resources
import { Model } from 'chrome-untrusted://resources/brave/candle_phi.bundle.js'

console.log('[Candle WASM] Initializing Mojo connection...')

// Initialize connection to the browser-side CandleService
const candleService = CandleService.getRemote()

console.log('[Candle WASM] CandleService remote obtained:', candleService)

// Implement the PhiInterface Mojo observer
class PhiInterfaceImpl {
  receiver: PhiInterfaceReceiver
  model: typeof Model | null
  isInitialized: boolean
  initTime: number

  constructor() {
    this.receiver = new PhiInterfaceReceiver(this)
    this.model = null
    this.isInitialized = false
    this.initTime = 0
  }

  // Implementation of PhiInterface::Init
  async init(
    modelFiles: LargeModelFiles,
    quantized: boolean,
  ): Promise<{ success: boolean }> {
    if (this.isInitialized) {
      console.log('Model already initialized')
      return { success: true }
    }

    const startTime = performance.now()
    console.log('Loading Phi model from provided files...')

    try {
      // Helper function to extract data from BigBuffer
      const extractBigBuffer = (
        buffer: any,
        name: string,
      ): Uint8Array | null => {
        if (buffer.bytes) {
          const data = new Uint8Array(buffer.bytes)
          console.log(`${name} using inline bytes, size:`, data.byteLength)
          return data
        } else if (buffer.sharedMemory) {
          const sharedMem = buffer.sharedMemory
          console.log(`${name} shared memory size:`, sharedMem.size)
          const mapResult = sharedMem.bufferHandle.mapBuffer(0, sharedMem.size)
          if (!mapResult.buffer) {
            console.error(
              `Failed to map ${name} shared buffer, result:`,
              mapResult.result,
            )
            return null
          }
          const data = new Uint8Array(mapResult.buffer)
          console.log(`${name} using shared memory, size:`, data.byteLength)
          return data
        } else {
          console.error(`Invalid ${name} BigBuffer: no bytes or shared memory`)
          return null
        }
      }

      // Extract all model files from BigBuffer
      console.log('Extracting model files from BigBuffer...')
      const weightsData = extractBigBuffer(modelFiles.weights, 'Weights')
      const tokenizerData = extractBigBuffer(modelFiles.tokenizer, 'Tokenizer')
      const configData = extractBigBuffer(modelFiles.config, 'Config')

      if (!weightsData || !tokenizerData || !configData) {
        console.error('Failed to extract model files')
        return { success: false }
      }

      console.log('Creating Phi Model instance...')
      // The WASM Model constructor expects:
      // new Model(weights, tokenizer, config, quantized)
      this.model = new Model(weightsData, tokenizerData, configData, quantized)
      this.isInitialized = true
      this.initTime = performance.now() - startTime
      console.log(
        `Phi model loaded successfully in ${this.initTime.toFixed(2)}ms`,
      )
      return { success: true }
    } catch (error) {
      console.error('Failed to load model:', error)
      return { success: false }
    }
  }

  // Implementation of PhiInterface::InitWithPrompt
  async initWithPrompt(
    prompt: string,
    temp: number,
    topP: number,
    repeatPenalty: number,
    repeatLastN: number,
    seed: bigint,
  ): Promise<{ firstToken: string }> {
    if (!this.isInitialized || !this.model) {
      console.error('Model not initialized')
      return { firstToken: '' }
    }

    try {
      console.log('Initializing with prompt:', prompt)
      const firstToken = this.model.init_with_prompt(
        prompt,
        temp,
        topP,
        repeatPenalty,
        repeatLastN,
        seed,
      )
      console.log('First token generated:', firstToken)
      return { firstToken }
    } catch (error) {
      console.error('Error initializing with prompt:', error)
      return { firstToken: '' }
    }
  }

  // Implementation of PhiInterface::NextToken
  async nextToken(): Promise<{ token: string; isEnd: boolean }> {
    if (!this.isInitialized || !this.model) {
      console.error('Model not initialized')
      return { token: '', isEnd: true }
    }

    try {
      const token = this.model.next_token()
      const isEnd = token === '<|endoftext|>'
      return { token: isEnd ? '' : token, isEnd }
    } catch (error) {
      console.error('Error generating next token:', error)
      return { token: '', isEnd: true }
    }
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Create and register the PhiInterface implementation
console.log('[Candle WASM] Creating PhiInterface implementation...')
const phiImpl = new PhiInterfaceImpl()
console.log('[Candle WASM] Binding PhiInterface to CandleService...')
candleService.bindPhi(phiImpl.getPendingRemote())

console.log('[Candle WASM] Phi WASM bridge initialized!')

// Expose to window for debugging
;(window as any).phiImpl = phiImpl
console.log('[Candle WASM] Exposed phiImpl for debugging')

// UI handling
const statusEl = document.getElementById('status')!
const promptInput = document.getElementById(
  'promptInput',
) as HTMLTextAreaElement
const generateBtn = document.getElementById('generateBtn') as HTMLButtonElement
const stopBtn = document.getElementById('stopBtn') as HTMLButtonElement
const clearBtn = document.getElementById('clearBtn') as HTMLButtonElement
const resultEl = document.getElementById('result')!
const statsEl = document.getElementById('stats')!

// Control inputs
const temperatureInput = document.getElementById(
  'temperature',
) as HTMLInputElement
const tempOutput = document.getElementById('tempOutput')!
const topPInput = document.getElementById('topP') as HTMLInputElement
const topPOutput = document.getElementById('topPOutput')!
const repeatPenaltyInput = document.getElementById(
  'repeatPenalty',
) as HTMLInputElement
const repeatPenaltyOutput = document.getElementById('repeatPenaltyOutput')!
const maxTokensInput = document.getElementById('maxTokens') as HTMLInputElement
const maxTokensOutput = document.getElementById('maxTokensOutput')!
const seedInput = document.getElementById('seed') as HTMLInputElement
const randomSeedBtn = document.getElementById('randomSeedBtn')!

// Update outputs when sliders change
temperatureInput.addEventListener('input', () => {
  tempOutput.textContent = parseFloat(temperatureInput.value).toFixed(2)
})
topPInput.addEventListener('input', () => {
  topPOutput.textContent = parseFloat(topPInput.value).toFixed(2)
})
repeatPenaltyInput.addEventListener('input', () => {
  repeatPenaltyOutput.textContent = parseFloat(
    repeatPenaltyInput.value,
  ).toFixed(2)
})
maxTokensInput.addEventListener('input', () => {
  maxTokensOutput.textContent = maxTokensInput.value
})

// Random seed button
randomSeedBtn.addEventListener('click', () => {
  seedInput.value = Math.floor(
    Math.random() * Number.MAX_SAFE_INTEGER,
  ).toString()
})

// Poll for initialization (model will be loaded by browser)
const checkInit = setInterval(() => {
  if (phiImpl.isInitialized) {
    statusEl.textContent = `Ready! (Initialized in ${phiImpl.initTime.toFixed(2)}ms)`
    statusEl.className = 'status ready'
    generateBtn.disabled = false
    clearInterval(checkInit)
  }
}, 100)

// Generation state
let abortGeneration = false

// Handle generate button click
generateBtn.addEventListener('click', async () => {
  const prompt = promptInput.value.trim()
  if (!prompt || !phiImpl.isInitialized) return

  abortGeneration = false
  generateBtn.disabled = true
  stopBtn.disabled = false

  const temp = parseFloat(temperatureInput.value)
  const topP = parseFloat(topPInput.value)
  const repeatPenalty = parseFloat(repeatPenaltyInput.value)
  const seed = BigInt(seedInput.value)
  const maxTokens = parseInt(maxTokensInput.value)

  statusEl.textContent = 'Generating...'
  statusEl.className = 'status generating'
  resultEl.style.display = 'block'
  statsEl.style.display = 'block'
  resultEl.textContent = prompt

  const startTime = performance.now()
  let tokenCount = 0

  try {
    // Initialize with prompt
    const initResult = await phiImpl.initWithPrompt(
      prompt,
      temp,
      topP,
      repeatPenalty,
      64,
      seed,
    )

    let fullText = prompt + initResult.firstToken
    resultEl.textContent = fullText
    tokenCount++

    // Generate tokens
    while (tokenCount < maxTokens && !abortGeneration) {
      const result = await phiImpl.nextToken()

      if (result.isEnd || !result.token) {
        break
      }

      fullText += result.token
      resultEl.textContent = fullText
      tokenCount++

      const elapsedTime = performance.now() - startTime
      const tokensPerSec = (tokenCount / elapsedTime) * 1000
      statsEl.textContent = `Tokens: ${tokenCount} | Time: ${(elapsedTime / 1000).toFixed(2)}s | Speed: ${tokensPerSec.toFixed(2)} tok/s`

      // Allow UI to update
      await new Promise((resolve) => setTimeout(resolve, 0))
    }

    const totalTime = performance.now() - startTime
    const tokensPerSec = (tokenCount / totalTime) * 1000
    statsEl.textContent = `Complete! Tokens: ${tokenCount} | Time: ${(totalTime / 1000).toFixed(2)}s | Speed: ${tokensPerSec.toFixed(2)} tok/s`
    statusEl.textContent = abortGeneration ? 'Stopped' : 'Complete'
    statusEl.className = 'status ready'
  } catch (error) {
    console.error('Generation error:', error)
    statusEl.textContent = `Error: ${error}`
    statusEl.className = 'status error'
  } finally {
    generateBtn.disabled = false
    stopBtn.disabled = true
  }
})

// Handle stop button
stopBtn.addEventListener('click', () => {
  abortGeneration = true
  stopBtn.disabled = true
})

// Handle clear button
clearBtn.addEventListener('click', () => {
  promptInput.value = ''
  resultEl.style.display = 'none'
  statsEl.style.display = 'none'
  resultEl.textContent = ''
  statsEl.textContent = ''
})

// Handle example button clicks
document.querySelectorAll('.example-btn').forEach((btn) => {
  btn.addEventListener('click', () => {
    const text = btn.getAttribute('data-text')
    if (text) {
      promptInput.value = text
    }
  })
})
