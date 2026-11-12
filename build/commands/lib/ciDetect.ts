// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const isTeamcity = process.env.TEAMCITY_VERSION !== undefined

export function detectCI(env: NodeJS.ProcessEnv = process.env): boolean {
  // TODO(http://brave.dev/b/56449): Experimental implementation for integration
  // with our bots generator.
  // A recipe sets this; what CI still needs comes from its builder instead.
  if (env.BRAVE_RECIPES_ENGINE !== undefined) {
    return false
  }
  return env.BUILD_ID !== undefined || env.TEAMCITY_VERSION !== undefined
}

export const isCI = detectCI()

// Matches depot_tools' `gclient_utils.AI_AGENT_ENV_VARS`.
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
