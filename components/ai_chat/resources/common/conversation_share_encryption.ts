// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The AES-GCM IV length (in bytes) recommended by the spec. The viewer expects
// the IV to be the first 12 bytes of the ciphertext blob.
const IV_LENGTH = 12

/**
 * Encodes bytes as standard base64. The viewer decodes the ciphertext blob with
 * a standard (non-url-safe) base64 decoder.
 */
function base64Encode(bytes: Uint8Array): string {
  let binary = ''
  for (const byte of bytes) {
    binary += String.fromCharCode(byte)
  }
  return btoa(binary)
}

/**
 * Encodes bytes as URL-safe base64 without padding. The viewer decodes the key
 * fragment with a base64url decoder.
 */
function base64UrlEncode(bytes: Uint8Array): string {
  return base64Encode(bytes)
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=+$/, '')
}

export interface EncryptedShare {
  /**
   * The AES-GCM ciphertext, prefixed with the IV and standard-base64 encoded.
   * This is uploaded to the sharing server.
   */
  ciphertext: string

  /**
   * The raw AES-GCM key, URL-safe-base64 encoded. This is never sent to the
   * server - it only ever leaves the client as the URL fragment the user
   * shares, so the server cannot decrypt the conversation.
   */
  keyFragment: string
}

/**
 * Encrypts `plaintext` with a freshly generated AES-GCM key so it can be shared
 * via the Brave sharing server without the server being able to read it. The
 * returned ciphertext is uploaded to the server; the key is only ever placed in
 * the URL fragment. See the `decryptShare` viewer routine for the inverse
 * operation this must remain compatible with.
 */
export async function encryptForSharing(
  plaintext: string,
): Promise<EncryptedShare> {
  const key = await crypto.subtle.generateKey(
    { name: 'AES-GCM', length: 256 },
    true,
    ['encrypt'],
  )

  const iv = crypto.getRandomValues(new Uint8Array(IV_LENGTH))
  // Copy the encoded plaintext into a fresh ArrayBuffer-backed array so it
  // satisfies WebCrypto's BufferSource parameter type (TextEncoder.encode
  // returns a Uint8Array<ArrayBufferLike>).
  const encoded: Uint8Array<ArrayBuffer> = new Uint8Array(
    new TextEncoder().encode(plaintext),
  )
  const ciphertext = await crypto.subtle.encrypt(
    { name: 'AES-GCM', iv },
    key,
    encoded,
  )

  // The viewer expects a single blob of `iv || ciphertext(+tag)`.
  const blob = new Uint8Array(iv.length + ciphertext.byteLength)
  blob.set(iv, 0)
  blob.set(new Uint8Array(ciphertext), iv.length)

  const rawKey = new Uint8Array(await crypto.subtle.exportKey('raw', key))

  return {
    ciphertext: base64Encode(blob),
    keyFragment: base64UrlEncode(rawKey),
  }
}
