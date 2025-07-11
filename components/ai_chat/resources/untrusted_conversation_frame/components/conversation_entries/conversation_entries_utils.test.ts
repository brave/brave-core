// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  getReasoningText,
  removeReasoning,
  removeCitationsWithMissingLinks,
} from './conversation_entries_utils'

describe('getReasoningText', () => {
  it('Should extract reasoning text between start and end tags', () => {
    const input = '<think>Reasoning text here.</think>'
    expect(getReasoningText(input)).toBe('Reasoning text here.')
  })

  it('Should handle missing end tag by returning the rest of the text', () => {
    const input = '<think>Start of reasoning without end.'
    expect(getReasoningText(input)).toBe('Start of reasoning without end.')
  })

  it('Should handle missing start tag by returning an empty string', () => {
    const input = 'Reasoning text with no start tag.</think>'
    expect(getReasoningText(input)).toBe('')
  })

  it('Should handle nested think tags and remove them from the string', () => {
    const input =
      '<think>Reasoning text <think>with nested</think> tags.</think>'
    expect(getReasoningText(input)).toBe('Reasoning text with nested tags.')
  })

  it('Should handle removing white space around reasoning text', () => {
    const input = '<think> Reasoning text here. </think>'
    expect(getReasoningText(input)).toBe('Reasoning text here.')
  })
})

describe('removeReasoning', () => {
  it('Should remove reasoning text between start and end tags', () => {
    const input = '<think>Reasoning text here.</think> Rest of the text.'
    expect(removeReasoning(input)).toBe(' Rest of the text.')
  })

  it('Should not fail if there is an empty string', () => {
    const input = ''
    expect(removeReasoning(input)).toBe('')
  })

  it('Should not fail if there is no reasoning text', () => {
    const input = 'Rest of the text.'
    expect(removeReasoning(input)).toBe('Rest of the text.')
  })

  it('should not fail if there is not ending tag', () => {
    const input = '<think>Reasoning text here.'
    expect(removeReasoning(input)).toBe('<think>Reasoning text here.')
  })

  it('should not fail if there is not starting tag', () => {
    const input = 'Reasoning text here.</think>'
    expect(removeReasoning(input)).toBe('Reasoning text here.</think>')
  })
})

describe('removeCitationsWithMissingLinks', () => {
  it('should remove citations with missing links', () => {
    const input = 'Citation [1] and [2] thats it[3].'
    const citationLinks = [
      'https://example.com/link1',
      'https://example.com/link2',
    ]
    expect(removeCitationsWithMissingLinks(input, citationLinks)).toBe(
      'Citation [1] and [2] thats it.',
    )
  })
})
