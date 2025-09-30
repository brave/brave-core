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
    output?: any[],
  ): Mojom.ToolUseEvent => ({
    toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
    id: '123',
    argumentsJson,
    output,
  })

  const createMockUIObserver = (
    memoryListener?: (memories: string[]) => void,
  ) => {
    let listener = memoryListener
    return {
      onMemoriesChanged: {
        addListener: jest.fn().mockImplementation((callback) => {
          listener = callback
          return 1
        }),
      },
      removeListener: jest.fn(),
      fireMemoryChange: (memories: string[]) => listener?.(memories),
    }
  }

  test('should not render if no memory content', () => {
    const { container } = render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent('{"invalid": "json"}')}
        />
      </MockContext>,
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
            output: undefined,
          }}
        />
      </MockContext>,
    )

    expect(container.innerHTML).toBe('')
  })

  test('should render error state when tool output contains error text', async () => {
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })

    render(
      <MockContext
        uiHandler={
          {
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('Error occurred')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-error')).toBeInTheDocument()
    })
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
  })

  test('should render success state when memory exists', async () => {
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })
    const mockUIObserver = createMockUIObserver()

    render(
      <MockContext
        uiHandler={
          {
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event')).toBeInTheDocument()
    })
    expect(
      screen.getByText(/CHAT_UI_MEMORY_UPDATED_WITH_CONTENT_LABEL/),
    ).toBeInTheDocument()
    expect(screen.getByTestId('memory-undo-button')).toBeInTheDocument()
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
    expect(mockHasMemory).toHaveBeenCalledWith('Test memory content')
  })

  test('should render undone state when memory does not exist', async () => {
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: false })
    const mockUIObserver = createMockUIObserver()

    render(
      <MockContext
        uiHandler={
          {
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })
    expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
    expect(screen.queryByTestId('memory-undo-button')).not.toBeInTheDocument()
    expect(mockHasMemory).toHaveBeenCalledWith('Test memory content')
  })

  test('should call deleteMemory when undo button is clicked', async () => {
    const mockDeleteMemory = jest.fn().mockResolvedValue(undefined)
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })
    const mockUIObserver = createMockUIObserver()

    render(
      <MockContext
        uiHandler={
          {
            deleteMemory: mockDeleteMemory,
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-undo-button')).toBeInTheDocument()
    })

    const undoButton = screen.getByTestId('memory-undo-button')
    await act(async () => {
      fireEvent.click(undoButton)
    })

    expect(mockDeleteMemory).toHaveBeenCalledWith('Test memory content')
  })

  test('should show undone state after undo button is clicked', async () => {
    const mockUIObserver = createMockUIObserver()
    const mockDeleteMemory = jest.fn().mockImplementation(async () => {
      // Simulate memory deletion by calling the listener
      // Empty array means memory was deleted
      mockUIObserver.fireMemoryChange([])
    })
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })

    render(
      <MockContext
        uiHandler={
          {
            deleteMemory: mockDeleteMemory,
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-undo-button')).toBeInTheDocument()
    })

    const undoButton = screen.getByTestId('memory-undo-button')

    await act(async () => {
      fireEvent.click(undoButton)
    })

    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })

    expect(screen.queryByTestId('memory-undo-button')).not.toBeInTheDocument()
  })

  test('should call openAIChatCustomizationSettings when manage button is clicked', async () => {
    const mockOpenSettings = jest.fn()
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })
    const mockUIObserver = createMockUIObserver()

    render(
      <MockContext
        uiHandler={
          {
            openAIChatCustomizationSettings: mockOpenSettings,
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-manage-button')).toBeInTheDocument()
    })

    const manageButton = screen.getByTestId('memory-manage-button')
    fireEvent.click(manageButton)

    expect(mockOpenSettings).toHaveBeenCalled()
  })

  test('should handle invalid JSON gracefully', () => {
    const { container } = render(
      <MockContext>
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent('{"memory": invalid json}', [
            createTextContentBlock(''),
          ])}
        />
      </MockContext>,
    )

    expect(container.innerHTML).toBe('')
  })

  test('should not allow undo if memory content is empty', () => {
    const mockDeleteMemory = jest.fn()

    render(
      <MockContext
        uiHandler={
          {
            deleteMemory: mockDeleteMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent('{"memory": ""}', [
            createTextContentBlock(''),
          ])}
        />
      </MockContext>,
    )

    expect(screen.queryByTestId('memory-tool-event')).not.toBeInTheDocument()
  })

  test('should not allow undo if already undone', async () => {
    const mockUIObserver = createMockUIObserver()
    const mockDeleteMemory = jest.fn().mockImplementation(async () => {
      // Empty array means memory was deleted
      mockUIObserver.fireMemoryChange([])
    })
    const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })

    render(
      <MockContext
        uiHandler={
          {
            deleteMemory: mockDeleteMemory,
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <MemoryToolEvent
          toolUseEvent={createToolUseEvent(
            '{"memory": "Test memory content"}',
            [createTextContentBlock('')],
          )}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByTestId('memory-undo-button')).toBeInTheDocument()
    })

    const undoButton = screen.getByTestId('memory-undo-button')

    // Click undo once
    await act(async () => {
      fireEvent.click(undoButton)
    })

    // Wait for state change to undone
    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })

    // Verify undo was only called once even if we try to click again
    expect(mockDeleteMemory).toHaveBeenCalledTimes(1)
  })

  describe('defensive JSON handling for memory field', () => {
    test('should handle undefined input', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should handle null input', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('null', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should handle empty object input', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should handle valid string memory', async () => {
      const mockHasMemory = jest.fn().mockResolvedValue({ exists: true })
      const mockUIObserver = createMockUIObserver()

      render(
        <MockContext
          uiHandler={
            {
              hasMemory: mockHasMemory,
            } as unknown as Mojom.UntrustedUIHandlerRemote
          }
          uiObserver={
            mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
          }
        >
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": "valid string"}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByTestId('memory-tool-event')).toBeInTheDocument()
      })
      expect(mockHasMemory).toHaveBeenCalledWith('valid string')
    })

    test('should handle empty string memory', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": ""}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should reject number memory type', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": 123}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should reject boolean memory type', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": true}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should reject null memory value', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": null}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should reject object memory type', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": {"key": "value"}}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })

    test('should reject array memory type', () => {
      const { container } = render(
        <MockContext>
          <MemoryToolEvent
            toolUseEvent={createToolUseEvent('{"memory": ["item1", "item2"]}', [
              createTextContentBlock(''),
            ])}
          />
        </MockContext>,
      )
      expect(container.innerHTML).toBe('')
    })
  })
})
