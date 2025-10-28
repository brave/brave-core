// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { stringifyContent, ContentNode } from './editable_content'

function createSkillNode(id: string, text: string): ContentNode {
  return {
    type: 'skill',
    id,
    text,
  }
}

describe('editable_content utilities', () => {
  describe('stringifyContent', () => {
    it('converts content to strings', () => {
      expect(stringifyContent(['Hello', ' ', 'world'])).toBe('Hello world')
      expect(
        stringifyContent(['Hello ', createSkillNode('1', 'Alice'), '!']),
      ).toBe('Hello Alice!')
      expect(stringifyContent([])).toBe('')
      expect(stringifyContent(['', 'Hi', '', 'there', ''])).toBe('Hithere')
    })
  })
})
