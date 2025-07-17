/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Crypto from '@cardano-sdk/crypto'
import { Cardano } from '@cardano-sdk/core'
import {
  AddressType,
  InMemoryKeyAgent,
  cip8,
} from '@cardano-sdk/key-management'
import { XorShift } from 'xorshift'
import { createHash } from 'crypto'
import fs from 'node:fs/promises'
import { dummyLogger } from 'ts-log'

// Generates CIP8/CIP30 randomized test vectors based on "@cardano-sdk/key-management implementation:
// npm init -y && npm i typescript tsx @cardano-sdk/core @cardano-sdk/crypto @cardano-sdk/key-management @types/node xorshift && npx tsx index.mts

const testCases = 1000

const makeRng = (seed: string) => {
  return new XorShift([
    ...new Uint32Array(createHash('sha256').update(seed).digest().buffer).slice(
      0,
      4,
    ),
  ])
}

const randomUint32 = (rng: XorShift): number => {
  return rng.randomint()[0]
}

const randomKeyIndex = (rng: XorShift): number => {
  return randomUint32(rng) % 100000
}

const randomPayload = (rng: XorShift) => {
  return Buffer.from(
    [...Array(randomUint32(rng) % 1024)].map(() => randomUint32(rng) % 256),
  )
}

const bip32Ed25519 = await Crypto.SodiumBip32Ed25519.create()

export const testKeyAgent = async () => {
  return InMemoryKeyAgent.fromBip39MnemonicWords(
    {
      chainId: Cardano.ChainIds.Mainnet,
      getPassphrase: async () => {
        return new Uint8Array(0)
      },
      mnemonicWords:
        'abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about'.split(
          ' ',
        ),
    },
    {
      bip32Ed25519: bip32Ed25519,
      logger: dummyLogger,
    },
  )
}

let result: Array<{
  test: string
  keyIndex: number
  payload: string
  cip8Signature: object
}> = []
const keyAgent = await testKeyAgent()

for (let i = 0; i < testCases; i++) {
  const test = `brave ${i}`
  const rng = makeRng(test)

  const keyIndex = randomKeyIndex(rng)
  const payload = randomPayload(rng).toString('hex')

  const address = await keyAgent.deriveAddress(
    { index: keyIndex, type: AddressType.External },
    0,
  )

  const cip8Signature = await cip8.cip30signData(keyAgent, {
    knownAddresses: [address],
    payload,
    signWith: address.address,
  })

  result.push({
    test,
    keyIndex,
    payload,
    cip8Signature,
  })
}

await fs.writeFile('test_vectors.json', JSON.stringify(result, null, 2))
