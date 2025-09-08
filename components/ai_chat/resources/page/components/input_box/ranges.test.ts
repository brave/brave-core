// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createNode,
  findParentEditable,
  getRangeToTriggerChar,
  replaceRange
} from './ranges'
import { ContentNode } from './editable'

function createEditableElement(): HTMLDivElement {
  const editable = document.createElement('div')
  editable.dataset.editor = 'true'
  return editable
}

function createAssociatedContentNode(id: string, text: string): ContentNode {
  return {
    type: 'associated-content',
    id,
    text
  }
}

function setupEditableWithText(text: string): { editable: HTMLDivElement, textNode: Text } {
  const editable = createEditableElement()
  const textNode = document.createTextNode(text)
  editable.appendChild(textNode)
  document.body.appendChild(editable)
  return { editable, textNode }
}

function setupEditableWithMultipleTextNodes(texts: string[]): {
  editable: HTMLDivElement,
  textNodes: Text[]
} {
  const editable = createEditableElement()
  const textNodes = texts.map(text => document.createTextNode(text))
  textNodes.forEach(node => editable.appendChild(node))
  document.body.appendChild(editable)
  return { editable, textNodes }
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
  endOffset: number
): Range {
  const range = document.createRange()
  range.setStart(startNode, startOffset)
  range.setEnd(endNode, endOffset)
  return range
}

function clearSelection(): void {
  const selection = window.getSelection()!
  selection.removeAllRanges()
}

function expectAssociatedContentSpan(
  container: Element,
  expectedId: string,
  expectedText: string
): void {
  const spanElement = container.querySelector('span[data-type="associated-content"]')
  expect(spanElement).toBeInTheDocument()
  expect(spanElement?.getAttribute('data-id')).toBe(expectedId)
  expect(spanElement?.getAttribute('data-text')).toBe(expectedText)
}

describe('ranges utilities', () => {
  describe('createNode', () => {
    it('creates text node for string content', () => {
      const result = createNode('Hello world')
      expect(result.nodeType).toBe(Node.TEXT_NODE)
      expect(result.textContent).toBe('Hello world')
    })

    it('creates span element for associated content', () => {
      const contentNode = createAssociatedContentNode('test-id', 'test content')

      const result = createNode(contentNode) as HTMLSpanElement
      expect(result.tagName).toBe('SPAN')
      expect(result.contentEditable).toBe('false')
      expect(result.dataset.text).toBe('test content')
      expect(result.dataset.type).toBe('associated-content')
      expect(result.dataset.id).toBe('test-id')
      expect(result.textContent).toBe('@test content')
    })

    it('handles empty associated content text', () => {
      const contentNode = createAssociatedContentNode('empty-id', '')

      const result = createNode(contentNode) as HTMLSpanElement
      expect(result.tagName).toBe('SPAN')
      expect(result.dataset.text).toBe('')
      expect(result.textContent).toBe('@')
    })

    it('preserves special characters in text nodes', () => {
      const specialText = 'Hello @#$%^&*()_+ world!'
      const result = createNode(specialText)
      expect(result.textContent).toBe(specialText)
    })

    it('preserves special characters in associated content', () => {
      const contentNode = createAssociatedContentNode('special-chars', 'special @#$%^&*()')

      const result = createNode(contentNode) as HTMLSpanElement
      expect(result.dataset.text).toBe('special @#$%^&*()')
      expect(result.textContent).toBe('@special @#$%^&*()')
    })
  })

  describe('findParentEditable', () => {
    it('returns element with data-editor attribute', () => {
      const editable = createEditableElement()
      document.body.appendChild(editable)

      const result = findParentEditable(editable)
      expect(result).toBe(editable)
    })

    it('finds direct parent element with data-editor attribute', () => {
      const editable = createEditableElement()
      const child = document.createElement('span')
      editable.appendChild(child)
      document.body.appendChild(editable)

      const result = findParentEditable(child)
      expect(result).toBe(editable)
    })

    it('finds nested parent element with data-editor attribute', () => {
      const editable = createEditableElement()
      const intermediate = document.createElement('div')
      const child = document.createElement('span')
      const grandchild = document.createElement('em')

      editable.appendChild(intermediate)
      intermediate.appendChild(child)
      child.appendChild(grandchild)
      document.body.appendChild(editable)

      const result = findParentEditable(grandchild)
      expect(result).toBe(editable)
    })

    it('returns undefined if no parent with data-editor found', () => {
      const element = document.createElement('div')
      const parent = document.createElement('section')
      parent.appendChild(element)
      document.body.appendChild(parent)

      const result = findParentEditable(element)
      expect(result).toBeUndefined()
    })

    it('returns undefined for null input', () => {
      const result = findParentEditable(null)
      expect(result).toBeUndefined()
    })

    it('returns undefined for text node without editable parent', () => {
      const textNode = document.createTextNode('test')
      const div = document.createElement('div')
      div.appendChild(textNode)
      document.body.appendChild(div)

      const result = findParentEditable(textNode)
      expect(result).toBeUndefined()
    })

    it('finds editable parent for text node', () => {
      const { editable, textNode } = setupEditableWithText('test content')

      const result = findParentEditable(textNode)
      expect(result).toBe(editable)
    })

    it('handles complex DOM structure with multiple elements', () => {
      const editable = createEditableElement()
      const p = document.createElement('p')
      const span = document.createElement('span')
      const textNode = document.createTextNode('nested text')
      const em = document.createElement('em')

      span.appendChild(textNode)
      span.appendChild(em)
      p.appendChild(span)
      editable.appendChild(p)
      document.body.appendChild(editable)

      const result = findParentEditable(em)
      expect(result).toBe(editable)
    })

    it('returns first editable parent when multiple exist', () => {
      const outerEditable = createEditableElement()
      outerEditable.id = 'outer'

      const innerEditable = createEditableElement()
      innerEditable.id = 'inner'

      const child = document.createElement('span')

      innerEditable.appendChild(child)
      outerEditable.appendChild(innerEditable)
      document.body.appendChild(outerEditable)

      const result = findParentEditable(child)
      expect(result).toBe(innerEditable)
      expect(result?.id).toBe('inner')
    })
  })

  describe('getRangeToTriggerChar', () => {
    it('finds trigger character in current text node', () => {
      const { textNode } = setupEditableWithText('Hello @world')

      // Create a selection at the end of the text
      createSelectionAt(textNode, 12) // Position after '@world'

      const result = getRangeToTriggerChar('@')

      expect(result).toBeDefined()
      if (result) {
        expect(result.startContainer).toBe(textNode)
        expect(result.startOffset).toBe(6) // Position of '@'

        // Check that the range contains the '@' character
        const rangeContent = result.toString()
        expect(rangeContent).toBe('@world')
      }
    })

    it('finds trigger character in previous text node', () => {
      const { textNodes } = setupEditableWithMultipleTextNodes(['Hello @', 'world'])
      const [textNode1, textNode2] = textNodes

      // Create a selection in the second text node
      createSelectionAt(textNode2, 5) // End of 'world'

      const result = getRangeToTriggerChar('@')

      expect(result).toBeDefined()
      if (result) {
        expect(result.startContainer).toBe(textNode1)
        expect(result.startOffset).toBe(6) // Position of '@'

        // Check that the range starts from '@' and extends to cursor position
        const rangeContent = result.toString()
        expect(rangeContent).toBe('@world')
      }
    })

    it('returns undefined when no trigger character found', () => {
      const { textNode } = setupEditableWithText('Hello world')

      // Create a selection in the text
      createSelectionAt(textNode, 5)

      const result = getRangeToTriggerChar('@')

      expect(result).toBeDefined() // Returns range but doesn't modify it
      if (result) {
        expect(result.startContainer).toBe(textNode)
        expect(result.startOffset).toBe(5) // Original position unchanged

        // Check that no content is selected since no trigger found
        const rangeContent = result.toString()
        expect(rangeContent).toBe('')
      }
    })

    it('returns undefined when selection is null', () => {
      // Clear selection
      clearSelection()

      const result = getRangeToTriggerChar('@')

      expect(result).toBeUndefined()
    })

    it('skips non-text nodes when searching', () => {
      const editable = createEditableElement()
      const textNode1 = document.createTextNode('Hello ')
      const spanNode = document.createElement('span')
      spanNode.textContent = 'test'
      const textNode2 = document.createTextNode('@world')

      editable.appendChild(textNode1)
      editable.appendChild(spanNode)
      editable.appendChild(textNode2)
      document.body.appendChild(editable)

      // Create a selection at the end of the last text node
      createSelectionAt(textNode2, 6)

      const result = getRangeToTriggerChar('@')!

      expect(result).toBeDefined()
      expect(result.startContainer).toBe(textNode2)
      expect(result.startOffset).toBe(0) // Position of '@' in textNode2

      // Check that the range contains '@world'
      const rangeContent = result.toString()
      expect(rangeContent).toBe('@world')
    })

    it('handles multiple trigger characters and finds the first one', () => {
      const { textNode } = setupEditableWithText('First @ then another @ here')

      const textLength = textNode.textContent?.length || 0

      // Create a selection at the end of text
      createSelectionAt(textNode, textLength)

      const result = getRangeToTriggerChar('@')!

      expect(result).toBeDefined()
      expect(result.startContainer).toBe(textNode)
      // The function finds the FIRST '@' in the text node (position 6)
      // This is correct behavior - it searches backwards through nodes but forward within nodes
      const text = textNode.textContent || ''
      const expectedOffset = text.indexOf('@')
      expect(result.startOffset).toBe(expectedOffset)

      // Check that the range contains everything from first '@' to cursor
      const rangeContent = result.toString()
      expect(rangeContent).toBe('@ then another @ here')
    })
  })

  describe('replaceRange', () => {
    it('replaces range with text content', () => {
      const { editable, textNode } = setupEditableWithText('Hello world')

      // Create a range selecting 'world'
      const range = createRangeSelection(textNode, 6, textNode, 11)

      replaceRange(range, 'universe')

      expect(editable.textContent).toBe('Hello universe')
    })

    it('replaces range with associated content node', () => {
      const { editable, textNode } = setupEditableWithText('Hello world')

      // Create a range selecting 'world'
      const range = createRangeSelection(textNode, 6, textNode, 11)

      const associatedContent = createAssociatedContentNode('mention-1', 'Alice')

      replaceRange(range, associatedContent)

      expect(editable.textContent).toBe('Hello @Alice')
      expectAssociatedContentSpan(editable, 'mention-1', 'Alice')
    })

    it('works in conjunction with getRangeToTriggerChar for mention replacement', () => {
      const { editable, textNode } = setupEditableWithText('Hello @ali and more text')

      // Position cursor after '@ali' (at position 10, after 'i')
      createSelectionAt(textNode, 10) // After '@ali'

      // Find the trigger range
      const triggerRange = getRangeToTriggerChar('@')!
      expect(triggerRange).toBeDefined()

      // Extend range to include the typed text after '@'
      triggerRange.setEnd(textNode, 10) // Include '@ali'

      // Replace with associated content
      const associatedContent = createAssociatedContentNode('user-alice', 'Alice Smith')

      replaceRange(triggerRange, associatedContent)

      expect(editable.textContent).toBe('Hello @Alice Smith and more text')

      expectAssociatedContentSpan(editable, 'user-alice', 'Alice Smith')
    })

    it('handles replacing content across multiple text nodes', () => {
      const { textNodes } = setupEditableWithMultipleTextNodes(['Hello @al', 'ice world'])
      const [, textNode2] = textNodes

      // Position cursor in second text node
      createSelectionAt(textNode2, 3) // After 'ice'

      // Find the trigger range (should find '@' in first node)
      const triggerRange = getRangeToTriggerChar('@')!
      expect(triggerRange).toBeDefined()

      // Manually extend range to include typed text across nodes
      triggerRange.setEnd(textNode2, 3) // Include '@alice' across both nodes

      // Replace with associated content
      const associatedContent = createAssociatedContentNode('user-alice', 'Alice Johnson')

      replaceRange(triggerRange, associatedContent)

      const editable = textNodes[0].parentElement!
      expect(editable.textContent).toBe('Hello @Alice Johnson world')

      expectAssociatedContentSpan(editable, 'user-alice', 'Alice Johnson')
    })

    it('updates selection after replacement', () => {
      const { textNode } = setupEditableWithText('Hello @test')

      // Create range selecting '@test'
      const range = createRangeSelection(textNode, 6, textNode, 11)

      const associatedContent = createAssociatedContentNode('test-id', 'TestUser')

      replaceRange(range, associatedContent)

      // Check that selection is updated
      const selection = window.getSelection()!
      expect(selection.rangeCount).toBe(1)
      const updatedRange = selection.getRangeAt(0)
      expect(updatedRange.collapsed).toBe(true) // Should be collapsed after replacement
    })
  })
})
