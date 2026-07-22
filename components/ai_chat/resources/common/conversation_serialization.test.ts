// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable import/first */

// Set up loadTimeData mock BEFORE importing the module under test
;(window as any).loadTimeData = {
  getString: (key: string) => (key === 'braveVersion' ? '1.93.8' : ''),
}

import { describe, it, expect } from '@jest/globals'
import ComplexConversation from '../page/stories/conversations/multi_tool_multi_turn'
import {
  stringifyConversationData,
  parseConversationData,
  serializeConversationForSharing,
} from './conversation_serialization'

describe('conversation serialization', () => {
  it('should serialize and deserialize conversation data with bigint fields', () => {
    const serialized = stringifyConversationData(ComplexConversation)
    const deserialized = parseConversationData(serialized)
    expect(deserialized).toEqual(ComplexConversation)
  })

  it('provides a known shared conversation viewer export format', () => {
    const serialized = serializeConversationForSharing(ComplexConversation)
    const parsed = JSON.parse(serialized)

    expect(parsed).toHaveProperty('version')
    expect(parsed.version).toMatch(/^\d+\.\d+\.\d+$/)

    expect(parsed).toHaveProperty('data')
    const deserialized = parseConversationData(parsed.data)
    expect(deserialized).toEqual(ComplexConversation)
  })
})
