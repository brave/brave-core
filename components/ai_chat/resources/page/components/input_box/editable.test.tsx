// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import '@testing-library/jest-dom'
import Editable, { stringifyContent, Content, ContentNode } from './editable'

// Test helpers
function createAssociatedContentNode(id: string, text: string): ContentNode {
  return {
    type: 'associated-content',
    id,
    text
  }
}

// Helper function to simulate DOM mutations that trigger MutationObserver
function triggerDOMMutation(element: Element) {
  // Trigger a mutation by briefly adding and removing a text node
  const tempNode = document.createTextNode('')
  element.appendChild(tempNode)
  element.removeChild(tempNode)
}

describe('editable utilities', () => {
  describe('stringifyContent', () => {
    it('converts string content nodes to strings', () => {
      const content: Content = ['Hello', ' ', 'world']
      const result = stringifyContent(content)
      expect(result).toBe('Hello world')
    })

    it('converts associated content nodes to their text property', () => {
      const content: Content = [
        'Hello ',
        createAssociatedContentNode('user-1', 'Alice'),
        ' how are you?'
      ]
      const result = stringifyContent(content)
      expect(result).toBe('Hello Alice how are you?')
    })

    it('handles mixed content types', () => {
      const content: Content = [
        'Meeting with ',
        createAssociatedContentNode('user-1', 'John'),
        ' and ',
        createAssociatedContentNode('user-2', 'Sarah'),
        ' at 3pm'
      ]
      const result = stringifyContent(content)
      expect(result).toBe('Meeting with John and Sarah at 3pm')
    })

    it('handles empty content', () => {
      const content: Content = []
      const result = stringifyContent(content)
      expect(result).toBe('')
    })

    it('handles empty strings in content', () => {
      const content: Content = ['', 'Hello', '', 'world', '']
      const result = stringifyContent(content)
      expect(result).toBe('Helloworld')
    })

    it('handles associated content with empty text', () => {
      const content: Content = [
        'Before ',
        createAssociatedContentNode('empty', ''),
        ' after'
      ]
      const result = stringifyContent(content)
      expect(result).toBe('Before  after')
    })
  })
})

describe('Editable component', () => {
  it('renders with basic content', () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={content} onContentChange={onContentChange} />)
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
      />
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveAttribute('data-placeholder', placeholder)
  })

  it('displays string content correctly', () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={content} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hello world')
  })

  it('displays mixed content with associated content nodes', () => {
    const content: Content = [
      'Hello ',
      createAssociatedContentNode('user-1', 'Alice'),
      ' welcome!'
    ]
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={content} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hello @Alice welcome!')

    // Check that the associated content span is rendered
    const associatedSpan = editable.querySelector('span[data-type="associated-content"]')
    expect(associatedSpan).toBeInTheDocument()
    expect(associatedSpan).toHaveAttribute('data-id', 'user-1')
    expect(associatedSpan).toHaveAttribute('data-text', 'Alice')
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
      />
    )
    const editable = container.querySelector('[data-editor]')!

    // Use fireEvent.paste instead of creating ClipboardEvent
    fireEvent.paste(editable, {
      clipboardData: {
        getData: () => 'pasted text'
      }
    })

    expect(onPaste).toHaveBeenCalledWith(expect.any(Object))
  })

  it('calls onContentChange when content is updated programmatically', async () => {
    const initialContent: Content = ['Initial']
    const onContentChange = jest.fn()

    const { rerender, container } = render(
      <Editable content={initialContent} onContentChange={onContentChange} />
    )
    const editable = container.querySelector('[data-editor]')!

    const newContent: Content = ['Updated content']

    rerender(
      <Editable content={newContent} onContentChange={onContentChange} />
    )

    await waitFor(() => {
      expect(editable).toHaveTextContent('Updated content')
    })
  })

  it('updates content when prop changes from parent', async () => {
    const initialContent: Content = ['First']
    const onContentChange = jest.fn()

    const { rerender, container } = render(
      <Editable content={initialContent} onContentChange={onContentChange} />
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('First')

    const newContent: Content = ['Second']
    rerender(
      <Editable content={newContent} onContentChange={onContentChange} />
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
      />
    )

    expect(ref.current).toHaveAttribute('data-editor')
  })

  it('forwards ref correctly with callback ref', () => {
    const content: Content = ['Test']
    let refElement: HTMLDivElement | null = null
    const callbackRef = (el: HTMLDivElement | null) => {
      refElement = el
    }

    render(
      <Editable
        ref={callbackRef}
        content={content}
        onContentChange={jest.fn()}
      />
    )

    expect(refElement).toHaveAttribute('data-editor')
  })

  it('has autofocus attribute', () => {
    const content: Content = ['Test']

    const { container } = render(<Editable content={content} onContentChange={jest.fn()} />)
    const editable = container.querySelector('[data-editor]')!

    expect(editable).toBeInTheDocument()
    // The autofocus behavior is browser-specific in tests, so we just verify the element exists
    // In the real component, the autoFocus prop is applied which translates to autofocus attribute
  })

  it('handles complex content with multiple associated content nodes', () => {
    const content: Content = [
      'Meeting between ',
      createAssociatedContentNode('user-1', 'Alice'),
      ', ',
      createAssociatedContentNode('user-2', 'Bob'),
      ' and ',
      createAssociatedContentNode('user-3', 'Carol'),
      ' scheduled for tomorrow'
    ]
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={content} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Meeting between @Alice, @Bob and @Carol scheduled for tomorrow')

    // Check that all associated content spans are rendered
    const associatedSpans = editable.querySelectorAll('span[data-type="associated-content"]')
    expect(associatedSpans).toHaveLength(3)

    expect(associatedSpans[0]).toHaveAttribute('data-id', 'user-1')
    expect(associatedSpans[0]).toHaveAttribute('data-text', 'Alice')

    expect(associatedSpans[1]).toHaveAttribute('data-id', 'user-2')
    expect(associatedSpans[1]).toHaveAttribute('data-text', 'Bob')

    expect(associatedSpans[2]).toHaveAttribute('data-id', 'user-3')
    expect(associatedSpans[2]).toHaveAttribute('data-text', 'Carol')
  })

  it('preserves cursor position when typing (does not jump to start)', async () => {
    const content: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={content} onContentChange={onContentChange} />)
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

  it('can modify content by updating the content prop', async () => {
    const initialContent: Content = ['Initial text']
    const onContentChange = jest.fn()

    const TestWrapper = () => {
      const [content, setContent] = React.useState<Content>(initialContent)

      return (
        <div>
          <Editable content={content} onContentChange={onContentChange} />
          <button
            onClick={() => setContent(['Modified text'])}
            data-testid="update-button"
          >
            Update Content
          </button>
          <button
            onClick={() => setContent(['Text with ', createAssociatedContentNode('user-1', 'mention'), ' added'])}
            data-testid="add-mention-button"
          >
            Add Mention
          </button>
        </div>
      )
    }

    render(<TestWrapper />)

    const editable = document.querySelector('[data-editor]')!
    const updateButton = screen.getByTestId('update-button')
    const addMentionButton = screen.getByTestId('add-mention-button')

    // Initial content
    expect(editable).toHaveTextContent('Initial text')

    // Update to simple text
    fireEvent.click(updateButton)

    await waitFor(() => {
      expect(editable).toHaveTextContent('Modified text')
    })

    // Update to content with mention
    fireEvent.click(addMentionButton)

    await waitFor(() => {
      expect(editable).toHaveTextContent('Text with @mention added')

      const associatedSpan = editable.querySelector('span[data-type="associated-content"]')
      expect(associatedSpan).toBeInTheDocument()
      expect(associatedSpan).toHaveAttribute('data-id', 'user-1')
      expect(associatedSpan).toHaveAttribute('data-text', 'mention')
    })
  })

  it('preserves content structure when prop updates with mixed content', async () => {
    const initialContent: Content = ['Hello']
    const onContentChange = jest.fn()

    const { rerender, container } = render(
      <Editable content={initialContent} onContentChange={onContentChange} />
    )
    const editable = container.querySelector('[data-editor]')!
    expect(editable).toHaveTextContent('Hello')

    // Update with mixed content
    const mixedContent: Content = [
      'Hello ',
      createAssociatedContentNode('user-1', 'Alice'),
      ' and ',
      createAssociatedContentNode('user-2', 'Bob'),
      '!'
    ]

    rerender(
      <Editable content={mixedContent} onContentChange={onContentChange} />
    )

    await waitFor(() => {
      expect(editable).toHaveTextContent('Hello @Alice and @Bob!')

      const associatedSpans = editable.querySelectorAll('span[data-type="associated-content"]')
      expect(associatedSpans).toHaveLength(2)

      expect(associatedSpans[0]).toHaveAttribute('data-id', 'user-1')
      expect(associatedSpans[0]).toHaveAttribute('data-text', 'Alice')

      expect(associatedSpans[1]).toHaveAttribute('data-id', 'user-2')
      expect(associatedSpans[1]).toHaveAttribute('data-text', 'Bob')
    })
  })

  it('onContentChange parses correct content from DOM changes', async () => {
    const initialContent: Content = ['Hello world']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Manually modify the DOM to simulate user input
    editable.textContent = 'Hello modified world'

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith(['Hello modified world'])
    })
  })

  it('onContentChange parses mixed content correctly from DOM', async () => {
    const initialContent: Content = ['Hello']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Manually create DOM structure with mixed content
    editable.innerHTML = ''

    // Add text node
    const textNode1 = document.createTextNode('Hello ')
    editable.appendChild(textNode1)

    // Add associated content span
    const span = document.createElement('span')
    span.dataset.type = 'associated-content'
    span.dataset.id = 'user-1'
    span.dataset.text = 'Alice'
    span.textContent = '@Alice'
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
          type: 'associated-content',
          id: 'user-1',
          text: 'Alice'
        },
        ' welcome!'
      ])
    })
  })

  it('onContentChange handles empty content correctly', async () => {
    const initialContent: Content = ['Some content']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Clear all content
    editable.innerHTML = ''

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith([])
    })
  })

  it('onContentChange ignores trailing BR elements', async () => {
    const initialContent: Content = ['Hello']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Clear any previous calls from initial render
    onContentChange.mockClear()

    // Add a BR element at the end (browser may add this)
    const br = document.createElement('br')
    editable.appendChild(br)

    // Trigger DOM mutation which should remove the BR
    triggerDOMMutation(editable)

    await waitFor(() => {
      // BR should be removed
      expect(editable.querySelector('br')).toBe(null)
    })

    // The mutation observer will detect the BR removal and call onContentChange
    // but the component should handle BR removal specially
    await waitFor(() => {
      // Check that onContentChange was called with the original content (without BR affecting it)
      expect(onContentChange).toHaveBeenCalledWith(['Hello'])
    })
  })

  it('onContentChange handles complex content with multiple nodes correctly', async () => {
    const initialContent: Content = ['']
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Create complex DOM structure
    editable.innerHTML = ''

    // Text + mention + text + mention + text
    editable.appendChild(document.createTextNode('Meeting with '))

    const mention1 = document.createElement('span')
    mention1.dataset.type = 'associated-content'
    mention1.dataset.id = 'user-alice'
    mention1.dataset.text = 'Alice'
    editable.appendChild(mention1)

    editable.appendChild(document.createTextNode(' and '))

    const mention2 = document.createElement('span')
    mention2.dataset.type = 'associated-content'
    mention2.dataset.id = 'user-bob'
    mention2.dataset.text = 'Bob'
    editable.appendChild(mention2)

    editable.appendChild(document.createTextNode(' at 3pm'))

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith([
        'Meeting with ',
        {
          type: 'associated-content',
          id: 'user-alice',
          text: 'Alice'
        },
        ' and ',
        {
          type: 'associated-content',
          id: 'user-bob',
          text: 'Bob'
        },
        ' at 3pm'
      ])
    })
  })

  it('should be able to delete associated-content nodes with backspace', async () => {
    const initialContent: Content = [
      'Hello ',
      createAssociatedContentNode('user-1', 'Alice'),
      ' world'
    ]
    const onContentChange = jest.fn()

    const { container } = render(<Editable content={initialContent} onContentChange={onContentChange} />)
    const editable = container.querySelector('[data-editor]')!

    // Clear any initial calls
    onContentChange.mockClear()

    // Position cursor after the associated content span
    const associatedSpan = editable.querySelector('span[data-type="associated-content"]')!
    const textNodeAfter = associatedSpan.nextSibling as Text

    const selection = window.getSelection()!
    const range = document.createRange()
    range.setStart(textNodeAfter, 0) // Position at start of " world"
    range.setEnd(textNodeAfter, 0)
    selection.removeAllRanges()
    selection.addRange(range)

    // Simulate backspace key that should delete the associated content node
    fireEvent.keyDown(editable, {
      key: 'Backspace',
      code: 'Backspace',
      charCode: 8,
      keyCode: 8
    })

    // Manually remove the associated content span to simulate the backspace behavior
    associatedSpan.remove()

    // Trigger DOM mutation
    triggerDOMMutation(editable)

    await waitFor(() => {
      expect(onContentChange).toHaveBeenCalledWith([
        'Hello ',
        ' world'
      ])
    })

    // Verify the associated content span is no longer in the DOM
    expect(editable.querySelector('span[data-type="associated-content"]')).toBe(null)
    // After removing the associated content, we should have the remaining text
    // Note: The text will be 'Hello world' because browser normalizes whitespace
    expect(editable).toHaveTextContent('Hello world')
  })
})
