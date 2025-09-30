// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import { createTextContentBlock } from '../../../common/content_block'
import * as React from 'react'
import MockContext from '../../mock_untrusted_conversation_context'
import ToolEvent from './tool_event'

describe('ToolEventTodos', () => {
  const validTodoOutput = {
    status: 'success',
    total_todos: 4,
    current_todos: [
      {
        id: 'task-1',
        content: 'Implement authentication',
        status: 'completed',
      },
      { id: 'task-2', content: 'Create user interface', status: 'in_progress' },
      { id: 'task-3', content: 'Add unit tests', status: 'pending' },
      { id: 'task-4', content: 'Deploy to staging', status: 'cancelled' },
    ],
  }

  test('should render completed todo tool event with all status types', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(validTodoOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Check header shows correct completion count, ignoring cancelled item
    expect(screen.getByText(S.CHAT_UI_TODO_HEADER_LABEL)).toBeInTheDocument()

    // Check all todo items are rendered
    expect(screen.getByText('Implement authentication')).toBeInTheDocument()
    expect(screen.getByText('Create user interface')).toBeInTheDocument()
    expect(screen.getByText('Add unit tests')).toBeInTheDocument()
    expect(screen.getByText('Deploy to staging')).toBeInTheDocument()
  })

  test('should render in-progress todo tool event', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": true, "todos": []}',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Check loading state
    expect(screen.getByTestId('tool-todos-header-text')).toHaveTextContent(
      S.CHAT_UI_TODO_TOOL_UPDATING_LABEL,
    )
  })

  test('should handle empty todo list', () => {
    const emptyOutput = {
      status: 'success',
      total_todos: 0,
      current_todos: [],
    }

    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(emptyOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(
      screen.queryByTestId('tool-todos-header-text'),
    ).not.toBeInTheDocument()
  })

  test('should handle all completed todos', () => {
    const allCompletedOutput = {
      status: 'success',
      total_todos: 2,
      current_todos: [
        { id: 'task-1', content: 'First task', status: 'completed' },
        { id: 'task-2', content: 'Second task', status: 'completed' },
      ],
    }

    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [
              createTextContentBlock(JSON.stringify(allCompletedOutput)),
            ],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TODO_HEADER_LABEL)).toBeInTheDocument()
  })

  test('should handle malformed JSON output', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock('{"invalid": json}')],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Should fall back to progress state when JSON is invalid
    expect(screen.getByText('Invalid output')).toBeInTheDocument()
  })

  test('should handle missing current_todos property', () => {
    const incompleteOutput = {
      status: 'success',
      total_todos: 3,
      // missing current_todos
    }

    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(incompleteOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Should fall back to progress state when current_todos is missing
    expect(screen.getByText('Invalid output')).toBeInTheDocument()
  })

  test('should handle todos with missing or invalid properties', () => {
    const uglyOutput = {
      status: 'success',
      total_todos: 3,
      current_todos: [
        { id: 'task-1', content: 'Good task', status: 'completed' },
        { content: 'Missing ID task', status: 'pending' }, // missing id
        {
          id: 'task-3',
          content: 'Invalid status task',
          status: 'unknown_status',
        },
        { id: 'task-4', status: 'pending' }, // missing content
        null, // null todo item
        { id: 'task-5', content: '', status: 'cancelled' }, // empty content
      ],
    }

    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(uglyOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(screen.getByText('Invalid output')).toBeInTheDocument()
  })

  test('should apply correct CSS classes for different statuses', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(validTodoOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Check that completed and cancelled items have strikethrough classes
    const completedItem = screen.getByTestId('tool-todos-item-task-1')
    const cancelledItem = screen.getByTestId('tool-todos-item-task-4')
    const inProgressItem = screen.getByTestId('tool-todos-item-task-2')
    const pendingItem = screen.getByTestId('tool-todos-item-task-3')

    expect(completedItem).toHaveClass('todoItemCompleted')
    expect(cancelledItem).toHaveClass('todoItemCancelled')
    expect(pendingItem).toHaveClass('todoItemPending')
    expect(inProgressItem).toHaveClass('todoItemInProgress')
    expect(inProgressItem).not.toHaveClass('todoItemCompleted')
    expect(inProgressItem).not.toHaveClass('todoItemCancelled')
    expect(inProgressItem).not.toHaveClass('todoItemPending')
    expect(inProgressItem).not.toHaveClass('todoItemCompleted')
    expect(inProgressItem).not.toHaveClass('todoItemCancelled')
    expect(inProgressItem).not.toHaveClass('todoItemPending')
  })

  test('should handle non-array current_todos', () => {
    const invalidArrayOutput = {
      status: 'success',
      total_todos: 1,
      current_todos: 'not an array',
    }

    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [
              createTextContentBlock(JSON.stringify(invalidArrayOutput)),
            ],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Should fall back to progress state when current_todos is not an array
    expect(screen.getByText('Invalid output')).toBeInTheDocument()
  })

  test('should handle bad json input for inactive entries', () => {
    const { container } = render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": }{false, "todos": []}',
            output: undefined,
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )

    expect(container.innerHTML).toBe('')
  })

  test('should handle bad json input for active entries', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": }{false, "todos": []}',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(
      screen.getByText(S.CHAT_UI_TODO_TOOL_UPDATING_LABEL),
    ).toBeInTheDocument()
  })

  test('should use index as fallback key when todo id is missing', () => {
    const noIdOutput = {
      status: 'success',
      total_todos: 2,
      current_todos: [
        { id: '', content: 'First task without ID', status: 'pending' },
        { id: '', content: 'Second task without ID', status: 'completed' },
      ],
    }

    const { container } = render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'todo_write',
            id: 'todo123',
            argumentsJson: '{"merge": false, "todos": []}',
            output: [createTextContentBlock(JSON.stringify(noIdOutput))],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(screen.getByText('First task without ID')).toBeInTheDocument()
    expect(screen.getByText('Second task without ID')).toBeInTheDocument()
    expect(screen.getByText(S.CHAT_UI_TODO_HEADER_LABEL)).toBeInTheDocument()

    // Should not cause any React key warnings (would show in console)
    expect(container.querySelector('.todoList')).toBeInTheDocument()
  })
})
