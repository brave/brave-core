// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Log from './log.ts'

// Under TeamCity, progress and status go out as service messages instead.
jest.mock('./ciDetect.ts', () => ({
  ...jest.requireActual('./ciDetect.ts'),
  isTeamcity: false,
}))

describe('setQuiet', () => {
  let consoleLog: jest.SpiedFunction<typeof console.log>

  beforeEach(() => {
    consoleLog = jest.spyOn(console, 'log').mockImplementation(() => {})
  })

  afterEach(() => {
    consoleLog.mockRestore()
  })

  it('prints details for a person', () => {
    Log.setQuiet(undefined, 'sync', {})
    Log.detail('a.cc is touched.')
    expect(Log.isEnabledFor(Log.Level.DETAIL)).toBe(true)
    expect(consoleLog.mock.calls).toEqual([['a.cc is touched.']])
  })

  it('leaves details out under an AI agent, saying so once', () => {
    const processOn = jest.spyOn(process, 'on').mockReturnValue(process)
    Log.setQuiet(undefined, 'sync', { CLAUDECODE: '1', AI_AGENT: 'claude' })
    expect(processOn).toHaveBeenCalledTimes(1)
    expect(processOn).toHaveBeenCalledWith('exit', expect.any(Function))
    const onExit = processOn.mock.calls[0]![1] as (code: number) => void
    processOn.mockRestore()
    Log.detail('a.cc is touched.')
    expect(Log.isEnabledFor(Log.Level.DETAIL)).toBe(false)
    expect(consoleLog).toHaveBeenCalledTimes(1)
    expect(consoleLog).toHaveBeenCalledWith(
      expect.stringContaining('Detected AI agent env (CLAUDECODE, AI_AGENT).'),
    )

    onExit(0)
    expect(consoleLog).toHaveBeenLastCalledWith(
      'The sync command has finished successfully.',
    )
    onExit(1)
    expect(consoleLog).toHaveBeenLastCalledWith(
      'The sync command has finished with an error.',
    )
  })

  it('says nothing on exit when quiet was asked for', () => {
    const processOn = jest.spyOn(process, 'on').mockReturnValue(process)
    Log.setQuiet(true, 'sync', { CLAUDECODE: '1' })
    expect(processOn).not.toHaveBeenCalled()
    processOn.mockRestore()
  })

  it('ignores agent variables set to an empty or false value', () => {
    Log.setQuiet(undefined, 'sync', {
      CLAUDECODE: '',
      GEMINI_CLI: '0',
      AI_AGENT: 'False',
    })
    expect(Log.isEnabledFor(Log.Level.DETAIL)).toBe(true)
  })

  it('lets --no-quiet win over an AI agent', () => {
    Log.setQuiet(false, 'sync', { CLAUDECODE: '1' })
    Log.detail('a.cc is touched.')
    expect(Log.isEnabledFor(Log.Level.DETAIL)).toBe(true)
    expect(consoleLog.mock.calls).toEqual([['a.cc is touched.']])
  })

  it('is quiet with --quiet for a person', () => {
    Log.setQuiet(true, 'sync', {})
    Log.detail('a.cc is touched.')
    expect(Log.isEnabledFor(Log.Level.DETAIL)).toBe(false)
    expect(consoleLog).not.toHaveBeenCalled()
  })

  it('leaves out command echoes when quiet', () => {
    Log.setQuiet(true, 'sync', {})
    Log.command('/src', 'git', ['rev-parse', 'HEAD'])
    expect(consoleLog).not.toHaveBeenCalled()
  })

  it('leaves out progress markers when quiet', () => {
    Log.setQuiet(true, 'sync', {})
    Log.progressScope('apply patches', () => {})
    expect(consoleLog).not.toHaveBeenCalled()
  })

  it('echoes commands otherwise', () => {
    Log.setQuiet(false, 'sync', {})
    Log.command('/src', 'git', ['rev-parse', 'HEAD'])
    expect(consoleLog).toHaveBeenCalledWith(
      expect.stringContaining('rev-parse HEAD'),
    )
  })
})
