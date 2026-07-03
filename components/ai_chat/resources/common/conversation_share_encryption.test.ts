// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { webcrypto } from 'node:crypto'
import { describe, it, expect } from '@jest/globals'
import { encryptForSharing } from './conversation_share_encryption'

// jsdom provides crypto.getRandomValues but not crypto.subtle, so back it with
// Node's Web Crypto implementation (which the browser provides natively).
if (!globalThis.crypto?.subtle) {
  Object.defineProperty(globalThis, 'crypto', {
    value: webcrypto,
    configurable: true,
  })
}

// The following helpers mirror the sharing viewer's decryption routine, which
// lives outside this repository. They exist so the test can prove that
// encryptForSharing produces output the viewer can decrypt.
function base64Decode(base64: string): Uint8Array {
  const binary = atob(base64)
  const bytes = new Uint8Array(binary.length)
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i)
  }
  return bytes
}

function base64urlDecode(base64url: string): Uint8Array {
  const base64 = base64url.replace(/-/g, '+').replace(/_/g, '/')
  const padding =
    base64.length % 4 === 0 ? '' : '='.repeat(4 - (base64.length % 4))
  return base64Decode(base64 + padding)
}

async function decryptShare(
  ciphertextB64: string,
  keyFragment: string,
): Promise<string> {
  const keyBytes = base64urlDecode(keyFragment)
  const cryptoKey = await crypto.subtle.importKey(
    'raw',
    keyBytes,
    { name: 'AES-GCM' },
    false,
    ['decrypt'],
  )

  const blob = base64Decode(ciphertextB64)
  const iv = blob.slice(0, 12)
  const data = blob.slice(12)

  const plaintext = await crypto.subtle.decrypt(
    { name: 'AES-GCM', iv },
    cryptoKey,
    data,
  )

  return new TextDecoder().decode(plaintext)
}

describe('conversation share encryption', () => {
  it('produces ciphertext the viewer can decrypt', async () => {
    const plaintext = JSON.stringify({ version: '1.2.3', data: 'hello world' })
    const { ciphertext, keyFragment } = await encryptForSharing(plaintext)

    const decrypted = await decryptShare(ciphertext, keyFragment)
    expect(decrypted).toEqual(plaintext)
  })

  it('round-trips unicode content losslessly', async () => {
    const plaintext = 'emoji 🦁 and accents éàü and 日本語'
    const { ciphertext, keyFragment } = await encryptForSharing(plaintext)

    const decrypted = await decryptShare(ciphertext, keyFragment)
    expect(decrypted).toEqual(plaintext)
  })

  it('uses a fresh key and IV for each call', async () => {
    const plaintext = 'same input'
    const first = await encryptForSharing(plaintext)
    const second = await encryptForSharing(plaintext)

    // A random key and IV mean the ciphertext and key must differ each time.
    expect(first.ciphertext).not.toEqual(second.ciphertext)
    expect(first.keyFragment).not.toEqual(second.keyFragment)

    // Both must still decrypt back to the original plaintext.
    expect(await decryptShare(first.ciphertext, first.keyFragment)).toEqual(
      plaintext,
    )
    expect(await decryptShare(second.ciphertext, second.keyFragment)).toEqual(
      plaintext,
    )
  })

  it('emits a url-safe key fragment', async () => {
    const { keyFragment } = await encryptForSharing('content')
    // base64url must not contain '+', '/', or '=' padding.
    expect(keyFragment).toMatch(/^[A-Za-z0-9_-]+$/)
  })
})
