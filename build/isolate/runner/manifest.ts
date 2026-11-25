// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The isolate manifest, written by build/isolate/generator.

import fs from 'node:fs'
import path from 'node:path'

export type IsolateSuite = {
  name: string
  // Directory to run the suite from. Relative to the manifest file in the
  // JSON; readManifest() resolves it.
  cwd: string
  // Test executable, relative to `cwd`.
  binary: string
  args: string[]
  // Environment variables to set when they are unset or empty.
  envDefaults: Record<string, string>
}

export type IsolateManifest = {
  suites: IsolateSuite[]
}

function isStringArray(value: unknown): value is string[] {
  return Array.isArray(value) && value.every((v) => typeof v === 'string')
}

function isStringRecord(value: unknown): value is Record<string, string> {
  return (
    typeof value === 'object'
    && value !== null
    && !Array.isArray(value)
    && Object.values(value).every((v) => typeof v === 'string')
  )
}

function isSuite(value: unknown): value is IsolateSuite {
  if (typeof value !== 'object' || value === null) {
    return false
  }
  const suite = value as Record<string, unknown>
  return (
    typeof suite.name === 'string'
    && typeof suite.cwd === 'string'
    && typeof suite.binary === 'string'
    && isStringArray(suite.args)
    && isStringRecord(suite.envDefaults)
  )
}

export function parseManifest(json: string): IsolateManifest {
  const manifest = JSON.parse(json) as Partial<IsolateManifest> | null
  if (!Array.isArray(manifest?.suites) || !manifest.suites.every(isSuite)) {
    throw new Error('Malformed isolate manifest suites')
  }
  return manifest as IsolateManifest
}

export function readManifest(file: string): IsolateManifest {
  const manifest = parseManifest(fs.readFileSync(file, 'utf8'))
  const manifestDir = path.dirname(file)
  return {
    suites: manifest.suites.map((suite) => ({
      ...suite,
      cwd: path.resolve(manifestDir, suite.cwd),
    })),
  }
}
