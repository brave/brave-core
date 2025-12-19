// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import braveComponentsStrings from 'gen/components/grit/brave_components_webui_mock_strings'

const boolValues: Record<string, boolean> = {
  isMobile: true,
  isHistoryEnabled: true,
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
}

const stringKeys = Object.keys(braveComponentsStrings)
console.log('Setting up loadTimeData mock with keys:', stringKeys)

const global: any = window
global.loadTimeData = {
  getString(key: string) {
    const knownString =
      braveComponentsStrings[key as keyof typeof braveComponentsStrings]
    if (knownString) {
      return knownString
    }
    switch (key) {
      case 'richSearchWidgetsOrigin':
        return 'https://prod.browser-ai-includes.s.brave.app'
      default:
        console.warn(`No loadTimeData.getString for key: ${key}`)
        return key
    }
  },

  getBoolean(key: string) {
    const knownValue = boolValues[key]
    if (knownValue !== undefined) {
      return knownValue
    }
    console.warn(`No loadTimeData.getBoolean for key: ${key}`)
    return false
  },

  getInteger(key: string) {
    throw new Error(`No getInteger loadTimeData mock for key: ${key}`)
  },
}
