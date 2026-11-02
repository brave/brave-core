// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Log from './log.ts'
import { allPatchStatus } from './gitPatcherLog.ts'

const ok = { path: 'a.cc', patchPath: 'a.cc.patch', reason: 'NO_PATCH_INFO' }
const failed = { ...ok, path: 'b.cc', error: { message: 'does not apply' } }

describe('allPatchStatus', () => {
  let consoleLog: jest.SpiedFunction<typeof console.log>

  beforeEach(() => {
    consoleLog = jest.spyOn(console, 'log').mockImplementation(() => {})
  })

  afterEach(() => {
    consoleLog.mockRestore()
    Log.setQuiet(false, 'sync', {})
  })

  const printed = () =>
    consoleLog.mock.calls.map((c) => String(c[0])).join('\n')

  it('prints nothing when quiet and every patch applied', () => {
    Log.setQuiet(true, 'sync', {})
    allPatchStatus([ok, ok], 'Chromium')
    allPatchStatus([], 'Chromium')
    expect(consoleLog).not.toHaveBeenCalled()
  })

  it('still lists failed patches when quiet', () => {
    Log.setQuiet(true, 'sync', {})
    allPatchStatus([ok, failed], 'Chromium')
    expect(printed()).toContain('1 successful patches:')
    expect(printed()).toContain('b.cc')
    expect(printed()).not.toContain('a.cc')
  })

  it('lists every patch otherwise', () => {
    Log.setQuiet(false, 'sync', {})
    allPatchStatus([ok, failed], 'Chromium')
    expect(printed()).toContain('a.cc')
    expect(printed()).toContain('b.cc')
  })
})
