// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { describe, it } from 'node:test'
import { parseManifest, readManifest } from './manifest.ts'

const manifest = {
  suites: [
    {
      name: 'a',
      cwd: 'data/src/out/Release',
      binary: 'a',
      args: ['--a'],
      envDefaults: { A: 'a' },
    },
  ],
}

describe('parseManifest', () => {
  it('parses a valid manifest', () => {
    assert.deepEqual(parseManifest(JSON.stringify(manifest)), manifest)
  })

  it('rejects malformed suites', () => {
    assert.throws(
      () => parseManifest(JSON.stringify({ suites: [{ name: 'a' }] })),
      /Malformed isolate manifest suites/,
    )
    assert.throws(
      () => parseManifest(JSON.stringify({})),
      /Malformed isolate manifest suites/,
    )
  })
})

describe('readManifest', () => {
  it('resolves suite cwds relative to the manifest file', () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'isolate-manifest-'))
    try {
      const file = path.join(dir, 'manifest.json')
      fs.writeFileSync(file, JSON.stringify(manifest))
      assert.deepEqual(
        readManifest(file).suites[0]?.cwd,
        path.join(dir, 'data/src/out/Release'),
      )
    } finally {
      fs.rmSync(dir, { recursive: true, force: true })
    }
  })
})
