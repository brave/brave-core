// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import {
  ContainersStrings,
  TrafficControlStrings,
} from '../brave_generated_resources_webui_strings.js'
import {
  RuleOperationError,
  TrafficRule,
} from '../traffic_control.mojom-webui.js'

// Em dash placeholder for an unset value.
export const kUnsetLabel = '\u2014'

export function urlFilterOf(rule: TrafficRule): string {
  return rule.condition.urlFilter ?? ''
}

// Non-empty, non-comment lines from freeform url_filter text (used for
// matching/favicon; blank lines and `#` comments are ignored).
export function urlFilterPatternsOf(rule: TrafficRule): string[] {
  return urlFilterOf(rule)
    .split('\n')
    .map((line) => line.trim())
    .filter((line) => line.length > 0 && !line.startsWith('#'))
}

export function firstUrlFilterOf(rule: TrafficRule): string {
  return urlFilterPatternsOf(rule)[0] ?? ''
}

// Builds the compact label shown in the rules list from freeform url_filter
// text:
// - If the first non-empty line is a comment, it is used as the title (`#`
//   prefix is stripped).
// - Otherwise the first URL pattern is used. If there are more patterns,
//   a localized "$1 (and $2 more URLs)" string is used so RTL locales can
//   reorder the URL and count.
export function urlFilterListLabel(rule: TrafficRule): string {
  const nonEmptyLines = urlFilterOf(rule)
    .split('\n')
    .map((line) => line.trim())
    .filter((line) => line.length > 0)
  if (nonEmptyLines.length === 0) {
    return ''
  }

  const firstLine = nonEmptyLines[0]
  // Prefer a leading comment as a human-readable rule name.
  if (firstLine.startsWith('#')) {
    return firstLine.replace(/^#+\s*/, '') || firstLine
  }

  // No leading comment: summarize by the first pattern (+ count of extras).
  const patterns = nonEmptyLines.filter((line) => !line.startsWith('#'))
  if (patterns.length <= 1) {
    return patterns[0] ?? ''
  }
  return loadTimeData.getStringF(
    TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_URL_FILTER_AND_MORE,
    patterns[0],
    patterns.length - 1,
  )
}

export function createEmptyRule(): TrafficRule {
  return {
    id: '',
    enabled: true,
    condition: {
      urlFilter: '',
    },
    target: {
      containerId: null,
      temporaryContainer: false,
    },
  }
}

export function cloneRuleForEdit(rule: TrafficRule): TrafficRule {
  const clone = structuredClone(rule)
  // Normalize unset url_filter so the textarea always edits a string.
  clone.condition.urlFilter ??= ''
  return clone
}

export function ruleOperationErrorMessage(error: RuleOperationError): string {
  let errorMessage = ''
  switch (error) {
    // This is a user-facing error we expect in normal operation.
    case RuleOperationError.kInvalidUrlFilter:
      errorMessage = loadTimeData.getString(
        TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_ERROR_INVALID_URL_FILTER,
      )
      break
    default:
      // These are unexpected errors. Just display the enum name for debugging.
      errorMessage = RuleOperationError[error]
      break
  }
  // Use the existing containers error template. It's not related to containers,
  // so it's fine to use it here.
  return loadTimeData.getStringF(
    ContainersStrings.SETTINGS_CONTAINERS_ERROR_TEMPLATE,
    errorMessage,
  )
}
