// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { dumpBuildHangDiagnostics } from './buildDiagnostics.ts'
import Log from './logging.js'

jest.mock('./logging.js', () => ({
  error: jest.fn(),
}))

describe('dumpBuildHangDiagnostics', () => {
  let tempDir: string
  let consoleLogSpy: jest.SpyInstance

  beforeEach(() => {
    tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'ci-diagnostics-'))
    consoleLogSpy = jest.spyOn(console, 'log').mockImplementation(() => {})
    jest.clearAllMocks()
  })

  afterEach(() => {
    consoleLogSpy.mockRestore()
    jest.restoreAllMocks()
    fs.rmSync(tempDir, { recursive: true, force: true })
  })

  it('logs the contents of an existing diagnostics file', () => {
    const diagPath = path.join(tempDir, 'siso.INFO')
    fs.writeFileSync(diagPath, 'first line\nsecond line\n', 'utf8')

    dumpBuildHangDiagnostics(tempDir)

    expect(consoleLogSpy).toHaveBeenCalledWith(`--- tail of ${diagPath} ---`)
    expect(consoleLogSpy).toHaveBeenCalledWith('first line\nsecond line\n')
    expect(Log.error).not.toHaveBeenCalled()
  })

  it('reads a redirected diagnostics file when the local file is missing', () => {
    const redirectedTarget = path.join(tempDir, 'redirect-target.INFO')
    const redirectedMarker = path.join(tempDir, 'siso.exe.INFO.redirected')
    fs.writeFileSync(redirectedTarget, 'redirected output\n', 'utf8')
    fs.writeFileSync(redirectedMarker, `${redirectedTarget}\n`, 'utf8')

    dumpBuildHangDiagnostics(tempDir)

    expect(consoleLogSpy).toHaveBeenCalledWith(
      `--- tail of ${redirectedTarget} ---`,
    )
    expect(consoleLogSpy).toHaveBeenCalledWith('redirected output\n')
    expect(Log.error).not.toHaveBeenCalled()
  })

  it('logs an error when a redirected target does not exist', () => {
    const redirectedTarget = path.join(tempDir, 'missing.INFO')
    const redirectedMarker = path.join(tempDir, 'siso_output.redirected')
    fs.writeFileSync(redirectedMarker, redirectedTarget, 'utf8')

    dumpBuildHangDiagnostics(tempDir)

    expect(Log.error).toHaveBeenCalledWith(
      `Redirected file ${redirectedTarget} does not exist`,
    )
    expect(consoleLogSpy).not.toHaveBeenCalled()
  })

  it('logs read errors without throwing', () => {
    const diagPath = path.join(tempDir, 'siso.INFO')
    fs.writeFileSync(diagPath, 'content that will not be read', 'utf8')

    const realFs = jest.requireActual('node:fs') as typeof import('node:fs')
    jest.spyOn(fs, 'openSync').mockImplementation((...args) => {
      if (args[0] === diagPath) {
        throw new Error('open failed')
      }
      return realFs.openSync(...args)
    })

    dumpBuildHangDiagnostics(tempDir)

    expect(consoleLogSpy).toHaveBeenCalledWith(`--- tail of ${diagPath} ---`)
    expect(Log.error).toHaveBeenCalledWith('open failed')
  })

  it('prints only the final complete lines for oversized files', () => {
    const diagPath = path.join(tempDir, 'siso.INFO')
    const content = `${'a'.repeat(130 * 1024)}\nkept line 1\nkept line 2\n`
    fs.writeFileSync(diagPath, content, 'utf8')

    dumpBuildHangDiagnostics(tempDir)

    expect(consoleLogSpy).toHaveBeenCalledWith('kept line 1\nkept line 2\n')
    expect(Log.error).not.toHaveBeenCalled()
  })
})
