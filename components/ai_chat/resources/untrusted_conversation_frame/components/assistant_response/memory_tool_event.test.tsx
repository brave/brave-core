// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { render, screen, fireEvent, waitFor, act } from '@testing-library/react'
import { createTextContentBlock } from '../../../common/content_block'
import * as Mojom from '../../../common/mojom'
import * as React from 'react'
import MockContext from '../../mock_untrusted_conversation_context'
import MemoryToolEvent from './memory_tool_event'

describe('MemoryToolEvent', () => {
  const createToolUseEvent = (
    argumentsJson: string,
    output?: any[]
  ): Mojom.ToolUseEvent => ({
    toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
    id: '123',
    argumentsJson,
    output
  })

  test('should not render if no memory content', () => {
    const { container } = render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent('{"invalid": "json"}')}
        />
      </MockContext>
    )

    expect(container.innerHTML).toBe('')
  })

  test('should not render if tool has not completed (no output)', () => {
    const { container } = render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={{
            toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
            id: '123',
            argumentsJson: '{"memory": "Test memory content"}',
            output: undefined
          }}
        />
      </MockContext>
    )

    expect(container.innerHTML).toBe('')
  })

  test('should render error state when tool output contains error text', () => {
    render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('Error occurred')]
          )}
        />
      </MockContext>
    )

    expect(screen.getByTestId('memory-tool-event-error')).toBeInTheDocument()
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
  })

  test('should render success state when memory exists', () => {
    render(
      <MockContext
        memories={['Test memory content']}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    expect(screen.getByTestId('memory-tool-event')).toBeInTheDocument()
    expect(screen.getByText(/Test memory content/)).toBeInTheDocument()
    expect(screen.getByTestId('memory-undo-button')).toBeInTheDocument()
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
  })

  test(
    'should render undone state when memory does not exist in context',
    () => {
    render(
      <MockContext
        memories={[]}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
    expect(screen.queryByTestId('memory-undo-button')).not.toBeInTheDocument()
  })

  test('should call deleteMemory when undo button is clicked', async () => {
    const mockDeleteMemory = jest.fn().mockResolvedValue(undefined)

    render(
      <MockContext
        memories={['Test memory content']}
        uiHandler={{
          deleteMemory: mockDeleteMemory
        } as any}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    const undoButton = screen.getByTestId('memory-undo-button')
    await act(async () => {
      fireEvent.click(undoButton)
    })

    expect(mockDeleteMemory).toHaveBeenCalledWith('Test memory content')
  })

  test('should show undone state after undo button is clicked', async () => {
    const mockDeleteMemory = jest.fn().mockResolvedValue(undefined)

    render(
      <MockContext
        memories={['Test memory content']}
        uiHandler={{
          deleteMemory: mockDeleteMemory
        } as any}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    const undoButton = screen.getByTestId('memory-undo-button')

    await act(async () => {
      fireEvent.click(undoButton)
    })

    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })

    expect(screen.queryByTestId('memory-undo-button')).not.toBeInTheDocument()
  })

  test(
    'should call openAIChatCustomizationSettings when manage button is clicked',
    () => {
    const mockOpenSettings = jest.fn()

    render(
      <MockContext
        memories={['Test memory content']}
        uiHandler={{
          openAIChatCustomizationSettings: mockOpenSettings
        } as any}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    const manageButton = screen.getByTestId('memory-manage-button')
    fireEvent.click(manageButton)

    expect(mockOpenSettings).toHaveBeenCalled()
  })

  test('should handle invalid JSON gracefully', () => {
    const { container } = render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": invalid json}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    expect(container.innerHTML).toBe('')
  })

  test('should not allow undo if memory content is empty', () => {
    const mockDeleteMemory = jest.fn()

    render(
      <MockContext
        memories={['']}
        uiHandler={{
          deleteMemory: mockDeleteMemory
        } as any}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": ""}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    expect(screen.queryByTestId('memory-tool-event')).not.toBeInTheDocument()
  })

  test('should not allow undo if already undone', async () => {
    const mockDeleteMemory = jest.fn().mockResolvedValue(undefined)

    render(
      <MockContext
        memories={['Test memory content']}
        uiHandler={{
          deleteMemory: mockDeleteMemory
        } as any}
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')]
          )}
        />
      </MockContext>
    )

    const undoButton = screen.getByTestId('memory-undo-button')

    // Click undo once
    await act(async () => {
      fireEvent.click(undoButton)
    })

    // Wait for state change
    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })

    // Verify undo was only called once even if we try to click again
    expect(mockDeleteMemory).toHaveBeenCalledTimes(1)
  })
})
