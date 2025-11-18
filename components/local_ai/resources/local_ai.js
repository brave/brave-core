// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

console.log('[Local AI Internals] Script loaded')

const loadModelBtn = document.getElementById('loadModelBtn')
const loadStatus = document.getElementById('loadStatus')
const embeddingIframe = document.getElementById('embeddingIframe')

const weightsPathInput = document.getElementById('weightsPath')
const tokenizerPathInput = document.getElementById('tokenizerPath')
const configPathInput = document.getElementById('configPath')

// Will be populated when iframe sends default path
let defaultPathSet = false

loadModelBtn.addEventListener('click', () => {
  const weightsPath = document.getElementById('weightsPath').value.trim()
  const tokenizerPath = document.getElementById('tokenizerPath').value.trim()
  const configPath = document.getElementById('configPath').value.trim()

  if (!weightsPath || !tokenizerPath || !configPath) {
    loadStatus.textContent = 'Error: All file paths are required'
    loadStatus.style.backgroundColor = '#f8d7da'
    loadStatus.style.color = '#721c24'
    loadStatus.style.display = 'block'
    return
  }

  loadStatus.textContent = 'Loading model files...'
  loadStatus.style.backgroundColor = '#fff3cd'
  loadStatus.style.color = '#856404'
  loadStatus.style.display = 'block'
  loadModelBtn.disabled = true

  // Send message to the untrusted iframe to load the model
  embeddingIframe.contentWindow.postMessage(
    {
      type: 'loadModel',
      weightsPath: weightsPath,
      tokenizerPath: tokenizerPath,
      configPath: configPath,
    },
    'chrome-untrusted://candle-embedding-gemma-wasm',
  )

  console.log('[Local AI Internals] Sent loadModel message to iframe')
})

// Listen for response from the untrusted iframe
window.addEventListener('message', (event) => {
  if (event.origin !== 'chrome-untrusted://candle-embedding-gemma-wasm') {
    return
  }

  console.log('[Local AI Internals] Received message from iframe:', event.data)

  if (event.data.type === 'defaultModelPath') {
    // Populate input fields with default paths
    const defaultPath = event.data.path
    console.log('[Local AI Internals] Setting default model path:', defaultPath)

    // Helper to convert path string format
    const pathString =
      typeof defaultPath === 'string'
        ? defaultPath
        : defaultPath.path || defaultPath

    weightsPathInput.value = `${pathString}/model.safetensors`
    tokenizerPathInput.value = `${pathString}/tokenizer.json`
    configPathInput.value = `${pathString}/config.json`
    defaultPathSet = true
  } else if (event.data.type === 'loadModelResult') {
    if (event.data.success) {
      loadStatus.textContent =
        'Model loaded successfully! You can now use the embedding interface.'
      loadStatus.style.backgroundColor = '#d1e7dd'
      loadStatus.style.color = '#0f5132'
    } else {
      loadStatus.textContent = `Error loading model: ${event.data.error || 'Unknown error'}`
      loadStatus.style.backgroundColor = '#f8d7da'
      loadStatus.style.color = '#721c24'
    }
    loadStatus.style.display = 'block'
    loadModelBtn.disabled = false
  }
})
