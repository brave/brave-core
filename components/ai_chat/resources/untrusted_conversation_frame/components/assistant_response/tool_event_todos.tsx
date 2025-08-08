// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import type { ToolComponent } from './tool_event'
import styles from './tool_event.module.scss'

interface TodoItem {
  id: string
  content: string
  status: 'pending' | 'in_progress' | 'completed' | 'cancelled'
}

interface TodoToolOutput {
  current_todos?: TodoItem[]
}

const ToolEventTodos: ToolComponent = (props) => {
  const content = props.content

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
      const output = JSON.parse(props.toolUseEvent.output[0].textContentBlock.text) as any
      // Validate it's a TodoToolOutput
      if (typeof output !== 'object' || output === null || !('current_todos' in output)) {
        return false
      }
      if (!Array.isArray(output.current_todos)) {
        return false
      }
      for (const todo of output.current_todos) {
        if (typeof todo !== 'object' || todo === null || typeof todo.id !== 'string' || typeof todo.content !== 'string' || typeof todo.status !== 'string' || !['pending', 'in_progress', 'completed', 'cancelled'].includes(todo.status)) {
          return false
        }
      }
      return output as TodoToolOutput
    } catch (e) {
      return false
    }
  }, [props.toolUseEvent.output])

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'completed':
        return <Icon name='check-circle-outline' className={styles.todoCompleted} />
      case 'cancelled':
        return <Icon name='close-circle' className={styles.todoCancelled} />
      case 'in_progress':
        return <Icon name='radio-unchecked' className={styles.todoInProgress} />
      case 'pending':
      default:
        return <Icon name='radio-unchecked' className={styles.todoPending} />
    }
  }

  const getCompletedCount = (todos: TodoItem[]) => {
    return todos.filter(todo => todo.status === 'completed').length
  }

  if (props.toolUseEvent.output && todoOutput && todoOutput?.current_todos) {
    const todos = todoOutput.current_todos
    const completedCount = getCompletedCount(todos)
    const totalCount = todos.length

    content.toolText = (
      <div className={styles.todoContainer}>
        <div className={styles.todoHeader}>
          <Icon name='list' className={styles.todoHeaderIcon} />
          <span className={styles.todoHeaderText}>
            {completedCount} of {totalCount} done
          </span>
        </div>
        <div className={styles.todoList}>
          {todos.map((todo, index) => (
            <div key={todo.id || index} className={styles.todoItem}>
              <div className={styles.todoItemIcon}>
                {getStatusIcon(todo.status)}
              </div>
              <div className={styles.todoItemContent}>
                <span className={classnames(
                  styles.todoItemText,
                  todo.status === 'completed' && styles.todoItemCompleted,
                  todo.status === 'cancelled' && styles.todoItemCancelled
                )}>
                  {todo.content}
                </span>
              </div>
            </div>
          ))}
        </div>
      </div>
    )

    content.statusIcon = content.progressIcon = (
      <span data-testid='tool-todos-completed-icon'>
        <Icon name='list' />
      </span>
    )
  } else {
    // Tool in progress
    content.progressIcon = (
      <span data-testid='tool-todos-progress-icon'>
        <Icon name='list' />
      </span>
    )

    content.statusIcon = (
      <span data-testid='tool-todos-error-icon'>
        <Icon name='close' />
      </span>
    )

    content.toolText = (
      <div className={styles.todoContainer}>
        <div className={styles.todoHeader}>
          <Icon name='list' className={styles.todoHeaderIcon} />
          <span className={styles.todoHeaderText}>
            {todoOutput === false ? 'Invalid output' : 'Managing todos...'}
          </span>
        </div>
      </div>
    )
  }

  return props.children(content)
}

export default ToolEventTodos
