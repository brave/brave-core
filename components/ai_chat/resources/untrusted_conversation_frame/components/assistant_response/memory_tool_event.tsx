// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale, formatLocale } from '$web-common/locale'
import styles from './memory_tool_event.module.scss'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import * as Mojom from '../../../common/mojom'
import '../../../common/strings'

interface Props {
  toolUseEvent: Mojom.ToolUseEvent
}

const MemoryToolEvent: React.FC<Props> = ({ toolUseEvent }) => {
  const context = useUntrustedConversationContext()
  const [memoryExists, setMemoryExists] = React.useState<boolean | null>(null)

  // Parse the memory content from tool input
  const memoryContent = React.useMemo(() => {
    if (!toolUseEvent?.argumentsJson) return ''
    try {
      const input = JSON.parse(toolUseEvent.argumentsJson)
      return typeof input?.memory === 'string' ? input.memory : ''
    } catch (e) {
      return ''
    }
  }, [toolUseEvent?.argumentsJson])

  // Check if memory exists using HasMemory API and subscribe to changes
  React.useEffect(() => {
    if (!memoryContent) return

    // Initial check
    context.uiHandler?.hasMemory(memoryContent).then(({ exists }) => {
      setMemoryExists(exists)
    })

    // Subscribe to memory changes via UI observer
    const id = context.uiObserver?.onMemoriesChanged.addListener(
      (memories: string[]) => {
        const exists = memories.includes(memoryContent)
        setMemoryExists(exists)
      },
    )

    return () => {
      context.uiObserver?.removeListener(id!)
    }
  }, [memoryContent])

  // Non-empty string in textContentBlock indicates an error
  const hasError = !!toolUseEvent.output?.[0]?.textContentBlock?.text

  const handleUndo = async () => {
    if (!memoryContent) return

    await context.uiHandler?.deleteMemory(memoryContent)
  }

  const handleManageAll = () => {
    context.uiHandler?.openAIChatCustomizationSettings?.()
  }

  // Don't render if no memory content, tool hasn't completed, or still
  // checking memory existence initially.
  if (!memoryContent || !toolUseEvent.output || memoryExists === null) {
    return null
  }

  const getTestId = () => {
    if (hasError) return 'memory-tool-event-error'
    if (!memoryExists) return 'memory-tool-event-undone'
    return 'memory-tool-event'
  }

  return (
    <div
      className={styles.memoryToolEvent}
      data-testid={getTestId()}
    >
      <Icon
        name='database'
        className={styles.icon}
      />
      {hasError && (
        <div className={styles.actions}>
          <span className={styles.textError}>
            {getLocale(S.CHAT_UI_MEMORY_ERROR_LABEL)}
          </span>
          <button
            className={styles.button}
            onClick={handleManageAll}
            data-testid='memory-manage-button'
          >
            {getLocale(S.CHAT_UI_MEMORY_MANAGE_ALL_BUTTON_LABEL)}
          </button>
        </div>
      )}
      {!hasError && !memoryExists && (
        <div className={styles.actions}>
          <span
            className={styles.textUndone}
            title={memoryContent}
          >
            {getLocale(S.CHAT_UI_MEMORY_UNDONE_LABEL)}
          </span>
          <button
            className={styles.button}
            onClick={handleManageAll}
            data-testid='memory-manage-button'
          >
            {getLocale(S.CHAT_UI_MEMORY_MANAGE_ALL_BUTTON_LABEL)}
          </button>
        </div>
      )}
      {!hasError && memoryExists && (
        <div className={styles.actions}>
          <span>
            {formatLocale(S.CHAT_UI_MEMORY_UPDATED_WITH_CONTENT_LABEL, {
              $1: memoryContent,
            })}
          </span>
          <button
            className={styles.button}
            onClick={handleUndo}
            data-testid='memory-undo-button'
          >
            {getLocale(S.CHAT_UI_MEMORY_UNDO_BUTTON_LABEL)}
          </button>
          <button
            className={styles.button}
            onClick={handleManageAll}
            data-testid='memory-manage-button'
          >
            {getLocale(S.CHAT_UI_MEMORY_MANAGE_ALL_BUTTON_LABEL)}
          </button>
        </div>
      )}
    </div>
  )
}

export default MemoryToolEvent
