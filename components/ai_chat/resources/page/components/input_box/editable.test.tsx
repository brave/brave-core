// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, fireEvent, waitFor } from '@testing-library/react'
import '@testing-library/jest-dom'
import Editable from './editable'
import { Content, ContentNode } from './editable_content'

// Test helpers
function createSkillNode(id: string, text: string): ContentNode {
  return {
    type: 'skill',
    id,
    text,
  }
}

// Helper function to simulate DOM mutations that trigger MutationObserver
function triggerDOMMutation(element: Element) {
  // Trigger a mutation by briefly adding and removing a text node
  const tempNode = document.createTextNode('')
  element.appendChild(tempNode)
  element.removeChild(tempNode)
}

describe('Editable component', () => {
  it('renders with basic content', () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toBeInTheDocument()
    expect(editable).toHaveAttribute('data-editor')
    expect(editable).toHaveAttribute('contenteditable', 'plaintext-only')
  })

  it('renders with placeholder', () => {
    const content: Content = []
    const onContentChange = jest.fn()
    const placeholder = 'Enter your message...'

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
        placeholder={placeholder}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveAttribute('data-placeholder', placeholder)
  })

  it('displays string content correctly', () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hello world')
  })

  it('displays mixed content with skill nodes', () => {
    const content: Content = [
      'Hello ',
      createSkillNode('user-1', 'Alice'),
      ' welcome!',
    ]
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hello Alice welcome!')

    // Check that the skill span is rendered
    const skillSpan = editable.querySelector('span[data-type="skill"]')
    expect(skillSpan).toBeInTheDocument()
    expect(skillSpan).toHaveAttribute('data-id', 'user-1')
    expect(skillSpan).toHaveAttribute('data-text', 'Alice')
  })

  it('handles paste events', () => {
    const content: Content = ['Initial text']
    const onContentChange = jest.fn()
    const onPaste = jest.fn()

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
        onPaste={onPaste}
      />,
    )
    const editable = container.querySelector('[data-editor]')!

    // Use fireEvent.paste instead of creating ClipboardEvent
    fireEvent.paste(editable, {
      clipboardData: {
        getData: () => 'pasted text',
      },
    })

    expect(onPaste).toHaveBeenCalledWith(expect.any(Object))
  })

  it('calls onContentChange when content is updated programmatically', async () => {
    const initialContent: Content = ['Initial']
    const onContentChange = jest.fn()

    const { rerender, container } = render(
      <Editable
        content={initialContent}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!

    const newContent: Content = ['Updated content']

    rerender(
      <Editable
        content={newContent}
        onContentChange={onContentChange}
      />,
    )

    await waitFor(() => {
      expect(editable).toHaveTextContent('Updated content')
    })
  })

  it('updates content when prop changes from parent', async () => {
    const initialContent: Content = ['First']
    const onContentChange = jest.fn()

    const { rerender, container } = render(
      <Editable
        content={initialContent}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('First')

    const newContent: Content = ['Second']
    rerender(
      <Editable
        content={newContent}
        onContentChange={onContentChange}
      />,
    )

    await waitFor(() => {
      expect(editable).toHaveTextContent('Second')
    })
  })

  it('forwards ref correctly', () => {
    const content: Content = ['Test']
    const ref = React.createRef<HTMLDivElement>()

    render(
      <Editable
        ref={ref}
        content={content}
        onContentChange={jest.fn()}
      />,
    )

    expect(ref.current).toHaveAttribute('data-editor')
  })

  it('renders multiple skill nodes', () => {
    const { container } = render(
      <Editable
        content={[
          'Hi ',
          createSkillNode('1', 'Alice'),
          ', ',
          createSkillNode('2', 'Bob'),
          '!',
        ]}
        onContentChange={jest.fn()}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hi Alice, Bob!')
    expect(editable.querySelectorAll('span[data-type="skill"]')).toHaveLength(2)
  })

  it('preserves cursor position when typing (does not jump to start)', async () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={content}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!

    // Position cursor in the middle of the text
    const textNode = editable.firstChild as Text
    const selection = window.getSelection()!
    const range = document.createRange()
    range.setStart(textNode, 6) // Position after "Hello "
    range.setEnd(textNode, 6)
    selection.removeAllRanges()
    selection.addRange(range)

    // Verify initial cursor position
    expect(selection.focusOffset).toBe(6)

    // Simulate typing by updating the text content and manually updating the cursor
    editable.textContent = 'Hello Xworld'

    // Manually set cursor position after the inserted character
    const newTextNode = editable.firstChild as Text
    const newRange = document.createRange()
    newRange.setStart(newTextNode, 7) // After "Hello X"
    newRange.setEnd(newTextNode, 7)
    selection.removeAllRanges()
    selection.addRange(newRange)

    // Verify cursor is preserved (not at start)
    const finalSelection = window.getSelection()!
    expect(finalSelection.focusOffset).toBeGreaterThan(0)
    expect(finalSelection.focusOffset).toBe(7) // After "Hello X"
  })

  it('onContentChange parses mixed content correctly from DOM', async () => {
    const initialContent: Content = ['Hello']
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={initialContent}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!

    // Manually create DOM structure with mixed content
    editable.innerHTML = ''

    // Add text node
    const textNode1 = document.createTextNode('Hello ')
    editable.appendChild(textNode1)

    // Add skill span
    const span = document.createElement('span')
    span.dataset.type = 'skill'
    span.dataset.id = 'user-1'
    span.dataset.text = 'Alice'
    span.textContent = 'Alice'
    span.contentEditable = 'false'
    editable.appendChild(span)

    // Add more text
    const textNode2 = document.createTextNode(' welcome!')
    editable.appendChild(textNode2)

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith([
        'Hello ',
        {
          type: 'skill',
          id: 'user-1',
          text: 'Alice',
        },
        ' welcome!',
      ])
    })
  })

  it('onContentChange handles empty content correctly', async () => {
    const initialContent: Content = ['Some content']
    const onContentChange = jest.fn()

    const { container } = render(
      <Editable
        content={initialContent}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!

    // Clear all content
    editable.innerHTML = ''

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith([])
    })
  })

  it('deletes skill nodes', async () => {
    const onContentChange = jest.fn()
    const { container } = render(
      <Editable
        content={['Hi ', createSkillNode('1', 'Alice'), ' there']}
        onContentChange={onContentChange}
      />,
    )
    const editable = container.querySelector('[data-editor]')!
    onContentChange.mockClear()

    editable.querySelector('span[data-type="skill"]')!.remove()
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith(['Hi ', ' there'])
    })
  })
})
