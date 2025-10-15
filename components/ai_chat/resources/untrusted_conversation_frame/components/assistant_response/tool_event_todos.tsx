// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import classnames from '$web-common/classnames'
import { formatLocale, getLocale } from '$web-common/locale'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event_todos.module.scss'

interface TodoItem {
  id: string
  content: string
  status: 'pending' | 'in_progress' | 'completed' | 'cancelled'
}

interface TodoToolOutput {
  current_todos?: TodoItem[]
}

// Validates if an object is a valid TodoToolOutput and returns it if it is
function getAsPossibleTodoToolOutput(output: any): TodoToolOutput | null {
  if (
    typeof output !== 'object'
    || output === null
    || !('current_todos' in output)
  ) {
    return null
  }
  if (!Array.isArray(output.current_todos)) {
    return null
  }
  for (const todo of output.current_todos) {
    if (
      typeof todo !== 'object'
      || todo === null
      || typeof todo.id !== 'string'
      || typeof todo.content !== 'string'
      || typeof todo.status !== 'string'
      || !['pending', 'in_progress', 'completed', 'cancelled'].includes(
        todo.status,
      )
    ) {
      return null
    }
  }
  return output as TodoToolOutput
}

function StatusIcon(props: { status: TodoItem['status'] }) {
  if (props.status === 'in_progress') {
    return <ProgressRing />
  }

  switch (props.status) {
    case 'completed':
      return <Icon name='check-circle-outline' />
    case 'cancelled':
      return <Icon name='close-circle' />
    case 'pending':
    default:
      return <Icon name='radio-unchecked' />
  }
}

const ToolEventTodos: ToolComponent = (props) => {
  const content: ToolUseContent = {
    toolLabel: null,
    expandedContent: null,
  }

  // Parse the tool output to get current todos
  // type can be:
  // null = no output yet
  // false = invalid output
  // TodoToolOutput = valid output
  const todoOutput = React.useMemo(() => {
    if (!props.toolUseEvent.output?.[0]?.textContentBlock?.text) {
      return null
    }
    try {
      const output = JSON.parse(
        props.toolUseEvent.output[0].textContentBlock.text,
      )
      // Validate it's a TodoToolOutput
      const possibleTodoOutput = getAsPossibleTodoToolOutput(output)
      if (possibleTodoOutput) {
        return possibleTodoOutput
      }
      return false
    } catch (e) {
      return false
    }
  }, [props.toolUseEvent.output])

  if (props.toolUseEvent.output && todoOutput && todoOutput?.current_todos) {
    const todos = todoOutput.current_todos
    const totalCount = todos.filter(
      (todo) => todo.status !== 'cancelled',
    ).length
    const completedCount = todos.filter(
      (todo) => todo.status === 'completed',
    ).length

    content.expandedContent = (
      <div className={styles.todoContainer}>
        <div className={styles.todoHeader}>
          <Icon
            name='list-checks'
            className={styles.todoHeaderIcon}
          />
          {totalCount > 0 && (
            <span
              className={styles.todoHeaderText}
              data-testid='tool-todos-header-text'
            >
              {formatLocale(S.CHAT_UI_TODO_HEADER_LABEL, {
                $1: completedCount,
                $2: totalCount,
              })}
            </span>
          )}
        </div>

        <div className={styles.todoList}>
          {todos.map((todo, index) => (
            <div
              key={todo.id || index}
              data-testid={`tool-todos-item-${todo.id}`}
              className={classnames(
                styles.todoItem,
                todo.status === 'completed' && styles.todoItemCompleted,
                todo.status === 'cancelled' && styles.todoItemCancelled,
                todo.status === 'in_progress' && styles.todoItemInProgress,
                todo.status === 'pending' && styles.todoItemPending,
              )}
            >
              <div className={styles.todoItemIcon}>
                <StatusIcon status={todo.status} />
              </div>
              <div className={styles.todoItemContent}>
                <span className={styles.todoItemText}>{todo.content}</span>
              </div>
            </div>
          ))}
        </div>
      </div>
    )
  } else {
    // Tool in progress
    content.expandedContent = (
      <div className={styles.todoContainer}>
        <div className={styles.todoHeader}>
          <Icon
            name='list-checks'
            className={styles.todoHeaderIcon}
          />
          <span
            className={styles.todoHeaderText}
            data-testid='tool-todos-header-text'
          >
            {todoOutput === false
              ? // This is more of a debug state, since it shouldn't happen as we
                // control the output.
                'Invalid output'
              : // We're still receiving the data
                getLocale(S.CHAT_UI_TODO_TOOL_UPDATING_LABEL)}
          </span>
        </div>
      </div>
    )
  }

  return props.children(content)
}

export default ToolEventTodos
