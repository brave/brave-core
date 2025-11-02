// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import { ToolComponent } from './tool_event'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './tool_event_content_text_filter_generation.module.scss'

/**
 * Tool event content for text_filter_generation tool.
 * Displays the markdown response with code blocks and adds a "Create Filter" button
 * to automate the custom scriptlet and filter rule creation.
 */
const ToolEventContentTextFilterGeneration: ToolComponent = (props) => {
  console.log('[TextFilterGeneration] Component mounted/rendered', {
    toolName: props.toolUseEvent.toolName,
    hasOutput: !!props.toolUseEvent.output,
    outputLength: props.toolUseEvent.output?.length,
    isEntryActive: props.isEntryActive,
  })
// console.error('TOOL OUTPUT:', JSON.stringify(props.toolUseEvent.output, null, 2))

  const context = useUntrustedConversationContext()
  const [isCreating, setIsCreating] = React.useState(false)
  const [createResult, setCreateResult] = React.useState<{
    success: boolean
    message: string
  } | null>(null)

  // Get the AI's markdown response from the completion event
  // NOTE: props.toolUseEvent.output contains the PROMPT sent TO the AI, not the AI's response
  // The AI's actual markdown response is in a CompletionEvent in the same assistant entry
  // context.conversationHistory is automatically updated as streaming happens via conversationObserver!
  const markdownResponse = React.useMemo(() => {
    console.log('[TextFilterGeneration] Computing markdownResponse from context.conversationHistory')

    if (!context.conversationHistory || context.conversationHistory.length === 0) {
      console.error('[TextFilterGeneration] No conversation history available')
      return ''
    }

    // Find the latest assistant entry (should be the one containing this tool event)
    const assistantEntries = context.conversationHistory.filter(
      entry => entry.characterType === 1 // Mojom.CharacterType.ASSISTANT
    )

    if (assistantEntries.length === 0) {
      console.error('[TextFilterGeneration] No assistant entries in conversation history')
      return ''
    }

    const latestAssistantEntry = assistantEntries[assistantEntries.length - 1]
    console.log('[TextFilterGeneration] Latest assistant entry has', latestAssistantEntry.events?.length, 'events')

    // Find the completion event (AI's markdown response)
    // Backend merges streaming chunks into ONE completion event
    const completionEvent = latestAssistantEntry.events?.find(event => event.completionEvent)

    if (completionEvent?.completionEvent?.completion) {
      const completion = completionEvent.completionEvent.completion
      console.log('[TextFilterGeneration] Found completion, length:', completion.length)
      console.log('[TextFilterGeneration] First 100 chars:', completion.substring(0, 100))
      return completion
    }

    console.error('[TextFilterGeneration] No completion event found')
    return ''
  }, [context.conversationHistory])

  const handleCreateFilter = React.useCallback(async () => {
    console.log('[TextFilterGeneration] Button clicked!', {
      hasConversationHandler: !!context.conversationHandler,
      hasMarkdownResponse: !!markdownResponse,
      markdownLength: markdownResponse.length,
      isEntryActive: props.isEntryActive,
    })

    if (!context.conversationHandler) {
      console.error('[TextFilterGeneration] No conversation handler!')
      setCreateResult({
        success: false,
        message: 'Error: No conversation handler available',
      })
      return
    }

    if (!markdownResponse) {
      console.error('[TextFilterGeneration] No markdown response!')
      setCreateResult({
        success: false,
        message: 'Error: No markdown response available',
      })
      return
    }

    console.log('[TextFilterGeneration] Starting filter creation...')
    setIsCreating(true)
    setCreateResult(null)

    // Parse and log what will be extracted from the markdown
    console.error('=== MARKDOWN RESPONSE (FULL) ===')
    console.error(markdownResponse)
    console.error('=== END MARKDOWN ===\n')

    // Parse the markdown to show what will be extracted
    const jsMatch = markdownResponse.match(/```javascript\s*([\s\S]*?)```/)
    const nameMatch = markdownResponse.match(/\*\*Scriptlet name:\*\*[\s\S]*?```\s*([\s\S]*?)```/)
    const ruleMatch = markdownResponse.match(/\*\*Filter rule:\*\*[\s\S]*?```\s*([\s\S]*?)```/)

    console.error('=== PARSED DATA ===')
    console.error('JavaScript Code:', jsMatch ? jsMatch[1].trim() : 'NOT FOUND')
    console.error('\nScriptlet Name:', nameMatch ? nameMatch[1].trim() : 'NOT FOUND')
    console.error('\nFilter Rule:', ruleMatch ? ruleMatch[1].trim() : 'NOT FOUND')
    console.error('=== END PARSED DATA ===\n')

    try {
      console.log('[TextFilterGeneration] Calling createCustomFilter with markdown:', markdownResponse.substring(0, 100))
      const result = await context.conversationHandler.createCustomFilter(
        markdownResponse
      )
      console.log('[TextFilterGeneration] Got result:', result)
      setCreateResult({
        success: result.success,
        message: result.errorMessage || 'Filter created successfully! Please reload the page to see the changes.',
      })
    } catch (error) {
      console.error('[TextFilterGeneration] Error creating filter:', error)
      setCreateResult({
        success: false,
        message: `Error: ${error}`,
      })
    } finally {
      setIsCreating(false)
    }
  }, [context.conversationHandler, markdownResponse, props.isEntryActive])

  // Parse the markdown to extract scriptlet info for display
  const parsedInfo = React.useMemo(() => {
    if (!markdownResponse) return null

    const jsMatch = markdownResponse.match(/```javascript\s*([\s\S]*?)```/)
    const nameMatch = markdownResponse.match(/\*\*Scriptlet name:\*\*[\s\S]*?```\s*([\s\S]*?)```/)
    const ruleMatch = markdownResponse.match(/\*\*Filter rule:\*\*[\s\S]*?```\s*([\s\S]*?)```/)

    return {
      code: jsMatch ? jsMatch[1].trim() : null,
      scriptletName: nameMatch ? nameMatch[1].trim() : null,
      filterRule: ruleMatch ? ruleMatch[1].trim() : null
    }
  }, [markdownResponse])

  // Render directly with the output visible, not hidden in an expandable section
  // We want to show the tool output (the markdown code) and the button together

  console.log('[TextFilterGeneration] About to render, markdownResponse length:', markdownResponse.length)

  // DON'T display the tool output (it's verbose instructions for the AI)
  // The AI will see it and generate code in its completion
  // We just show a simple message with the "Create Filter" button
  const customContent = {
    ...props.content,
    toolLabel: null,
    expandedContent: (
      <div className={styles.container}>
        <div className={styles.infoMessage}>
          Leo is generating a custom filter for you. Once it's done, you can click the button below to automatically create it.
        </div>

        {parsedInfo && parsedInfo.scriptletName && (
          <details className={styles.details}>
            <summary className={styles.summary}>
              <Icon name='code-block' />
              View scriptlet details
            </summary>
            <div className={styles.detailsContent}>
              <div className={styles.detailItem}>
                <strong>Scriptlet name:</strong>
                <div className={styles.codeBlock}>
                  {parsedInfo.scriptletName}
                </div>
              </div>
              <div className={styles.detailItem}>
                <strong>Filter rule:</strong>
                <div className={styles.codeBlock}>
                  {parsedInfo.filterRule}
                </div>
              </div>
            </div>
          </details>
        )}

        <ConversationAreaButton
          onClick={() => {
            console.log('[TextFilterGeneration] Button clicked!')
            handleCreateFilter()
          }}
          isDisabled={isCreating || createResult?.success || !markdownResponse}
          isLoading={isCreating}
          icon={
            createResult?.success ? (
              <Icon name='check-circle-filled' />
            ) : (
              <Icon name='product-brave-leo' />
            )
          }
        >
          {createResult?.success
            ? 'Filter Created'
            : (!markdownResponse ? 'Waiting for response...' : 'Insert scriptlet')}
        </ConversationAreaButton>

        {createResult && (
          <div className={createResult.success ? styles.successMessage : styles.errorMessage}>
            {createResult.message}
          </div>
        )}
      </div>
    ),
  }

  console.log('[TextFilterGeneration] Rendering with custom content', {
    hasExpandedContent: !!customContent.expandedContent,
    hasToolLabel: !!customContent.toolLabel,
  })

  return props.children(customContent)
}

export default ToolEventContentTextFilterGeneration
