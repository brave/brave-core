/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Derives the per-account sync seed entirely on the client from the user's
// account credentials. The seed is the 32-byte symmetric key that identifies a
// sync chain in Brave Sync; every device signed into the same Brave Account
// derives the identical seed, so they automatically join the same chain.
//
// The derivation is intentionally independent of the OPAQUE session key: the
// sync server only ever stores the OPAQUE verifier and encrypted sync blobs,
// never the password or this seed, preserving end-to-end encryption.
//
// NOTE: `account_salt` should be the per-account public salt returned by the
// account server (login_init). When not provided (e.g. local/dev), a fixed
// application salt is used so behaviour is deterministic.
export async function deriveAccountSyncSeed(
  email: string,
  password: string,
  accountSalt?: string,
): Promise<string> {
  const enc = new TextEncoder()
  const material = enc.encode(`${email.trim().toLowerCase()}:${password}`)
  const salt = enc.encode(accountSalt ?? 'brave-account-sync-v1')

  const keyMaterial = await crypto.subtle.importKey(
    'raw',
    material,
    { name: 'PBKDF2' },
    false,
    ['deriveBits'],
  )
  const bits = await crypto.subtle.deriveBits(
    { name: 'PBKDF2', salt, iterations: 100000, hash: 'SHA-256' },
    keyMaterial,
    256,
  )
  const bytes = new Uint8Array(bits)
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join('')
}
