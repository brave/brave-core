// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PageHandler } from 'gen/brave/components/local_ai/common/local_ai_internals.mojom.m.js'

console.log('[Local AI Internals] Script loaded')

// Initialize Mojo connection to page handler
const pageHandler = PageHandler.getRemote()

const serviceStatus = document.getElementById('serviceStatus')!
const compareBtn = document.getElementById('compareBtn')! as HTMLButtonElement
const text1Input = document.getElementById('text1')! as HTMLInputElement
const text2Input = document.getElementById('text2')! as HTMLInputElement
const result = document.getElementById('result')!

// Update status to show service is shared
// Use DOM APIs instead of innerHTML to avoid Trusted Types violations
const strongEl = document.createElement('strong')
strongEl.textContent = '✓ CandleService is running'
const brEl = document.createElement('br')
const smallEl = document.createElement('small')
smallEl.textContent =
  'Shared per-profile service that automatically loads the model from '
  + 'component updater'

serviceStatus.textContent = ''
serviceStatus.appendChild(strongEl)
serviceStatus.appendChild(brEl)
serviceStatus.appendChild(smallEl)
serviceStatus.style.borderLeftColor = '#4caf50'
serviceStatus.style.background = '#e8f5e9'

// Enable the compare button
compareBtn.disabled = false

// Calculate cosine similarity between two vectors
function cosineSimilarity(vec1: number[], vec2: number[]): number {
  if (vec1.length !== vec2.length) {
    throw new Error('Vectors must have the same length')
  }

  let dotProduct = 0
  let norm1 = 0
  let norm2 = 0

  for (let i = 0; i < vec1.length; i++) {
    dotProduct += vec1[i] * vec2[i]
    norm1 += vec1[i] * vec1[i]
    norm2 += vec2[i] * vec2[i]
  }

  const magnitude = Math.sqrt(norm1) * Math.sqrt(norm2)
  if (magnitude === 0) {
    return 0
  }

  return dotProduct / magnitude
}

// Get similarity interpretation
function getSimilarityInterpretation(similarity: number) {
  if (similarity >= 0.9) {
    return { label: 'Very High', color: '#4caf50', bg: '#e8f5e9' }
  } else if (similarity >= 0.7) {
    return { label: 'High', color: '#8bc34a', bg: '#f1f8e9' }
  } else if (similarity >= 0.5) {
    return { label: 'Moderate', color: '#ff9800', bg: '#fff3e0' }
  } else if (similarity >= 0.3) {
    return { label: 'Low', color: '#ff5722', bg: '#fbe9e7' }
  } else {
    return { label: 'Very Low', color: '#f44336', bg: '#ffebee' }
  }
}

// Handle compare button click
compareBtn.addEventListener('click', async () => {
  const text1 = text1Input.value.trim()
  const text2 = text2Input.value.trim()

  if (!text1 || !text2) {
    result.textContent = 'Please enter both texts'
    result.style.display = 'block'
    result.style.background = '#ffebee'
    return
  }

  compareBtn.disabled = true
  result.style.display = 'none'
  const startTime = performance.now()

  try {
    // Call CandleService to generate embeddings for both texts
    const [result1, result2] = await Promise.all([
      pageHandler.generateEmbedding(text1),
      pageHandler.generateEmbedding(text2),
    ])

    const embedding1 = result1.embedding
    const embedding2 = result2.embedding

    if (!embedding1.length || !embedding2.length) {
      throw new Error('Failed to generate embeddings')
    }

    const embedTime = performance.now() - startTime

    // Calculate cosine similarity
    const similarity = cosineSimilarity(embedding1, embedding2)
    const interpretation = getSimilarityInterpretation(similarity)

    // Display results
    const text1Preview = `"${text1.substring(0, 50)}${
      text1.length > 50 ? '...' : ''
    }"`
    const text2Preview = `"${text2.substring(0, 50)}${
      text2.length > 50 ? '...' : ''
    }"`

    result.textContent = [
      `Text 1: ${text1Preview}`,
      `Text 2: ${text2Preview}`,
      ``,
      `Embedding dimensions: ${embedding1.length}`,
      `Processing time: ${embedTime.toFixed(0)}ms`,
      ``,
      `Cosine Similarity: ${similarity.toFixed(4)}`,
      `Interpretation: ${interpretation.label}`,
    ].join('\n')
    result.style.display = 'block'
    result.style.background = interpretation.bg
    result.style.borderLeftColor = interpretation.color
  } catch (error) {
    result.textContent = `Error: ${(error as Error).message || error}`
    result.style.display = 'block'
    result.style.background = '#ffebee'
    result.style.borderLeftColor = '#f44336'
  } finally {
    compareBtn.disabled = false
  }
})

// Handle example comparison buttons
document.querySelectorAll('.example-compare').forEach((btn) => {
  btn.addEventListener('click', () => {
    const text1 = btn.getAttribute('data-text1')
    const text2 = btn.getAttribute('data-text2')
    if (text1 && text2) {
      text1Input.value = text1
      text2Input.value = text2
      // Automatically trigger comparison
      compareBtn.click()
    }
  })
})

console.log('[Local AI Internals] CandleService debug UI ready')
