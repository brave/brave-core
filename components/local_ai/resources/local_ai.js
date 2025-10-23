// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

document.addEventListener('DOMContentLoaded', () => {
  const radios = document.querySelectorAll('input[name="model"]')

  const frames = {
    bert: document.getElementById('bert-frame'),
    gemma: document.getElementById('gemma-frame'),
    phi: document.getElementById('phi-frame'),
  }

  const frameUrls = {
    bert: 'chrome-untrusted://candle-bert-wasm/',
    gemma: 'chrome-untrusted://candle-embedding-gemma-wasm/',
    phi: 'chrome-untrusted://candle-phi-wasm/',
  }

  radios.forEach((radio) => {
    radio.addEventListener('change', (e) => {
      const selectedModel = e.target.value

      Object.keys(frames).forEach((model) => {
        const frame = frames[model]
        if (model === selectedModel) {
          frame.classList.add('active')
          frame.src = frameUrls[model]
        } else {
          frame.classList.remove('active')
          frame.src = 'about:blank'
        }
      })
    })
  })
})
