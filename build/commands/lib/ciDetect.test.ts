// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { activeAiAgentEnvVars } from './ciDetect.ts'

describe('activeAiAgentEnvVars', () => {
  it('lists the agent variables that are set', () => {
    expect(
      activeAiAgentEnvVars({ CLAUDECODE: '1', AI_AGENT: 'claude', HOME: '/h' }),
    ).toEqual(['CLAUDECODE', 'AI_AGENT'])
  })

  it('ignores variables set to an empty or false value', () => {
    expect(
      activeAiAgentEnvVars({
        CLAUDECODE: '',
        GEMINI_CLI: '0',
        AI_AGENT: 'False',
      }),
    ).toEqual([])
  })
})
