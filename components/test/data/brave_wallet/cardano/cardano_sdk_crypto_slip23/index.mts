/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Crypto from '@cardano-sdk/crypto';
import { HexBlob } from "@cardano-sdk/util";
import { XorShift } from "xorshift";
import { createHash } from "crypto";
import fs from "node:fs/promises";

// Generates Bip32Ed25519 test vectors based one "@cardano-sdk/crypto implementation:
// npm init -y && npm i typescript tsx @cardano-sdk/crypto @types/node xorshift && npx tsx index.mts

const testCases = 1000;
const entropySize = 32;
const maxPathLength = 8;

const makeRng = (seed: string) => {
  return new XorShift([
    ...new Uint32Array(createHash("sha256").update(seed).digest().buffer).slice(
      0,
      4
    ),
  ]);
};

const randomUint32 = (rng: XorShift): number => {
  return rng.randomint()[0];
};

const randomEntropy = (rng: XorShift) => {
  return Buffer.from(
    [...Array(entropySize)].map(() => randomUint32(rng) % 256)
  );
};

const randomPath = (rng: XorShift) => {
  return [
    ...[...Array(randomUint32(rng) % maxPathLength)].map(() =>
      randomUint32(rng)
    ),
  ];
};

const bip32Ed25519 = await Crypto.SodiumBip32Ed25519.create();

let result: Array<{
  test: string;
  entropy: string;
  path: number[];
  privatekey: string;
  pubkey: string;
  signature: string;
}> = [];

for (let i = 0; i < testCases; i++) {
  const test = `brave ${i}`;
  const rng = makeRng(test);

  const entropy = randomEntropy(rng);
  const path = randomPath(rng);
  const rootPrivateKey = bip32Ed25519.fromBip39Entropy(entropy, "");

  const privatekey = bip32Ed25519.derivePrivateKey(rootPrivateKey, path);
  const pubkey = bip32Ed25519.getBip32PublicKey(privatekey);

  const signature = bip32Ed25519.sign(
    bip32Ed25519.getRawPrivateKey(privatekey),
    Buffer.from("message").toString("hex") as HexBlob
  );

  result.push({
    test,
    entropy: entropy.toString("hex"),
    path,
    privatekey,
    pubkey,
    signature,
  });
}

await fs.writeFile('test_vectors.json', JSON.stringify(result, null, 2));
