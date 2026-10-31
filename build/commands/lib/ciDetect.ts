// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const isTeamcity = process.env.TEAMCITY_VERSION !== undefined

export const isCI = process.env.BUILD_ID !== undefined || isTeamcity

// The variables that tell an AI agent runs the command, as depot_tools'
// `gclient_utils.AI_AGENT_ENV_VARS` checks them.
const AI_AGENT_ENV_VARS = [
  'GEMINI_CLI',
  'CLAUDECODE',
  'ANTIGRAVITY_AGENT',
  'CODEX_SANDBOX',
  'CURSOR_AGENT',
  'COPILOT_CLI',
  'AI_AGENT',
]

export function activeAiAgentEnvVars(
  env: NodeJS.ProcessEnv = process.env,
): string[] {
  return AI_AGENT_ENV_VARS.filter((name) => {
    const value = env[name]
    return (
      value !== undefined && !['', '0', 'false'].includes(value.toLowerCase())
    )
  })
}

export const isAiAgent = activeAiAgentEnvVars().length > 0
