// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getModelIcon } from './conversation_history_utils'

describe('getModelIcon', () => {
  it('Should return correct icon for known model keys', () => {
    expect(getModelIcon('chat-automatic')).toBe('product-brave-leo')
    expect(getModelIcon('chat-deepseek-r1')).toBe('deepseek-color')
    expect(getModelIcon('chat-claude-instant')).toBe('anthropic-color')
    expect(getModelIcon('chat-claude-haiku')).toBe('anthropic-color')
    expect(getModelIcon('chat-claude-sonnet')).toBe('anthropic-color')
    expect(getModelIcon('chat-qwen')).toBe('qwen-color')
    expect(getModelIcon('chat-basic')).toBe('meta-color')
    expect(getModelIcon('chat-vision-basic')).toBe('meta-color')
  })

  it('Should return fallback icon for unknown model keys', () => {
    expect(getModelIcon('unknown-model')).toBe('product-brave-leo')
    expect(getModelIcon('')).toBe('product-brave-leo')
    expect(getModelIcon('chat-invalid')).toBe('product-brave-leo')
  })

  it('Should handle case sensitivity', () => {
    expect(getModelIcon('CHAT-AUTOMATIC')).toBe('product-brave-leo')
    expect(getModelIcon('Chat-Automatic')).toBe('product-brave-leo')
  })
})
