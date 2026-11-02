// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  stringifyContent,
  ContentNode,
  createContentFromDOMNodes,
  makeEdit,
} from './editable_content'

function createSkillNode(id: string, text: string): ContentNode {
  return {
    type: 'skill',
    id,
    text,
  }
}

function setupEditableWithText(text: string): {
  editable: HTMLDivElement
  textNode: Text
} {
  const editable = document.createElement('div')
  editable.dataset.editor = 'true'
  const textNode = document.createTextNode(text)
  editable.appendChild(textNode)
  document.body.appendChild(editable)
  return { editable, textNode }
}

function createSelectionAt(node: Node, offset: number): void {
  const selection = window.getSelection()!
  const range = document.createRange()
  range.setStart(node, offset)
  range.setEnd(node, offset)
  selection.removeAllRanges()
  selection.addRange(range)
}

function createRangeSelection(
  startNode: Node,
  startOffset: number,
  endNode: Node,
  endOffset: number,
): Range {
  const range = document.createRange()
  range.setStart(startNode, startOffset)
  range.setEnd(endNode, endOffset)
  return range
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

  describe('createContentFromDOMNodes', () => {
    const parseHTML = (html: string) => {
      const editable = document.createElement('div')
      editable.innerHTML = html
      return createContentFromDOMNodes(editable)
    }
    const parseText = (html: string) => stringifyContent(parseHTML(html))

    it('parses plain text', () => {
      expect(parseHTML('Hello world')).toEqual(['Hello world'])
    })

    it('parses content nodes', () => {
      expect(
        parseHTML(
          'Hi <span data-type="skill" data-id="1" data-text="/s">/s</span>!',
        ),
      ).toEqual(['Hi ', { type: 'skill', id: '1', text: '/s' }, '!'])
    })

    it('converts <br> to newlines', () => {
      expect(parseText('a<br>b')).toBe('a\nb')
      expect(parseText('a<br><br>b')).toBe('a\n\nb')
    })

    it('keeps an empty last line', () => {
      expect(parseText('a<br><br>')).toBe('a\n')
    })

    it('reads text inside block elements', () => {
      // WebKit wraps each line in a block when pasting multi-line text.
      expect(parseText('<div>a</div><div>b</div>')).toBe('a\nb')
      expect(parseText('a<div>b</div>')).toBe('a\nb')
      expect(parseText('<div>a</div>b')).toBe('a\nb')
      expect(parseText('<p>a</p><p>b</p>')).toBe('a\nb')
    })

    it('treats placeholder <br> in blocks as empty lines', () => {
      expect(parseText('<div>a</div><div><br></div><div>b</div>')).toBe(
        'a\n\nb',
      )
      expect(parseText('<div>a<br></div><div>b</div>')).toBe('a\nb')
    })

    it('collapses nested blocks', () => {
      expect(parseText('<div><div>a</div></div><div>b</div>')).toBe('a\nb')
    })

    it('ignores empty text nodes after the last block', () => {
      const editable = document.createElement('div')
      editable.innerHTML = '<div>a</div><div>b</div>'
      editable.appendChild(document.createTextNode(''))
      expect(stringifyContent(createContentFromDOMNodes(editable))).toBe('a\nb')
    })

    it('reads content nodes inside blocks', () => {
      expect(
        parseHTML(
          '<div>a</div><div><span data-type="skill" data-id="1" data-text="/s">/s</span> b</div>',
        ),
      ).toEqual(['a', '\n', { type: 'skill', id: '1', text: '/s' }, ' b'])
    })

    it('reads text inside unknown inline elements', () => {
      expect(parseText('a<span>b</span>c')).toBe('abc')
    })
  })

  describe('selectRangeToTriggerChar', () => {
    it('finds and selects trigger character', () => {
      const { editable, textNode } = setupEditableWithText('Hello /world')
      createSelectionAt(textNode, 12)
      makeEdit(editable).selectRangeToTriggerChar('/')
      const selection = window.getSelection()!
      const range = selection.getRangeAt(0)
      expect(range.startOffset).toBe(6)
      expect(range.toString()).toBe('/world')
    })

    it('does nothing when no selection', () => {
      const { editable } = setupEditableWithText('Hello /world')
      window.getSelection()?.removeAllRanges()
      makeEdit(editable).selectRangeToTriggerChar('/')
      // Should not throw
    })
  })

  describe('replaceSelectedRange', () => {
    it('replaces selected range with text', () => {
      const { editable, textNode } = setupEditableWithText('Hello world')
      const range = createRangeSelection(textNode, 6, textNode, 11)
      window.getSelection()!.removeAllRanges()
      window.getSelection()!.addRange(range)
      makeEdit(editable).replaceSelectedRange('universe')
      expect(editable.textContent).toBe('Hello universe')
    })

    it('replaces selected range with skill node', () => {
      const { editable, textNode } = setupEditableWithText('Hello world')
      const range = createRangeSelection(textNode, 6, textNode, 11)
      window.getSelection()!.removeAllRanges()
      window.getSelection()!.addRange(range)
      makeEdit(editable).replaceSelectedRange(createSkillNode('id', 'Alice'))
      expect(editable.textContent).toBe('Hello Alice')
      const spanElement = editable.querySelector('span[data-type="skill"]')
      expect(spanElement?.getAttribute('data-id')).toBe('id')
      expect(spanElement?.getAttribute('data-text')).toBe('Alice')
    })

    it('works with selectRangeToTriggerChar', () => {
      const { editable, textNode } = setupEditableWithText('Hi /skill here')
      createSelectionAt(textNode, 9)
      makeEdit(editable).selectRangeToTriggerChar('/')
      const range = window.getSelection()!.getRangeAt(0)
      range.setEnd(textNode, 9)
      window.getSelection()!.removeAllRanges()
      window.getSelection()!.addRange(range)
      makeEdit(editable).replaceSelectedRange(createSkillNode('id', 'Alice'))
      expect(editable.textContent).toBe('Hi Alice here')
    })
  })
})
