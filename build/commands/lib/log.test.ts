// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

describe('detail', () => {
  let consoleLog: jest.SpiedFunction<typeof console.log>

  beforeEach(() => {
    consoleLog = jest.spyOn(console, 'log').mockImplementation(() => {})
  })

  afterEach(() => {
    consoleLog.mockRestore()
    jest.resetModules()
  })

  async function loadLog(agentEnvVars: string[]) {
    jest.doMock('./ciDetect.ts', () => ({
      isTeamcity: false,
      isAiAgent: agentEnvVars.length > 0,
      activeAiAgentEnvVars: () => agentEnvVars,
    }))
    return import('./log.ts')
  }

  it('prints every line for a person', async () => {
    const Log = await loadLog([])
    Log.detail('a.cc is touched.')
    Log.detail('b.cc is touched.')
    expect(consoleLog.mock.calls).toEqual([
      ['a.cc is touched.'],
      ['b.cc is touched.'],
    ])
  })

  it('leaves the lines out for an AI agent, saying so once', async () => {
    const Log = await loadLog(['CLAUDECODE', 'AI_AGENT'])
    Log.detail('a.cc is touched.')
    Log.detail('b.cc is touched.')
    expect(consoleLog).toHaveBeenCalledTimes(1)
    expect(consoleLog).toHaveBeenCalledWith(
      expect.stringContaining('Detected AI agent env (CLAUDECODE, AI_AGENT).'),
    )
    expect(consoleLog).not.toHaveBeenCalledWith(expect.stringContaining('a.cc'))
  })
})
