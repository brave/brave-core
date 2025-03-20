// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
// TODO: move to common
import { SuggestionButtonRaw } from '../../../page/components/suggested_question/suggested_question_raw'
import styles from './style.module.scss'


export default function ToolEvent(props: { event: Mojom.ToolUseEvent, isActiveEntry: boolean }) {
  const context = useUntrustedConversationContext()
  const toolUse = props.event

  const input: any | null = React.useMemo(() => {
    if (!toolUse.inputJson) {
      return null
    }
    let input
    try {
      input = JSON.parse(toolUse.inputJson)
    } catch (e) {
      return null
    }
    return input
  }, [toolUse.inputJson])

  let toolText = <>{toolUse.toolName}</>
  let statusIcon = <Icon name="check-circle-outline" />
  let progressIcon = <ProgressRing />

  if (toolUse.toolName === 'computer') {
    switch (input?.action) {
      case 'screenshot': {
        toolText = <>Looking at the page</>
        break
      }
      case 'key': {
        toolText = <>Pressing the key: {input?.text}</>
        break
      }
      case 'type': {
        toolText = <div title={input?.text} className={styles.toolUseLongText}>Typing: "{input?.text}"</div>
        break
      }
      case 'mouse_move': {
        toolText = <>Moving the mouse</>
        break
      }
      case 'left_click': {
        toolText = <>Clicking the left mouse button</>
        break
      }
      case 'scroll': {
        toolText = <>Scrolling</>
        break
      }
      default: {
        toolText = <>{input?.action}</>
      }
    }

    // Append any images (e.g. screenshots) in the output as a tooltip
    const toolResultOutputImageContentBlocks = toolUse.output?.filter(content => !!content.imageContentBlock) ?? []

    if (toolResultOutputImageContentBlocks.length) {
      toolText = (
        <Tooltip>
          {toolText}
          <div slot='content'>
          {toolResultOutputImageContentBlocks.map((content, i) => (
            <div key={i}>
              <img className={styles.toolUseContentPreview} src={content.imageContentBlock?.imageUrl} />
            </div>
          ))}
          </div>
        </Tooltip>
      )
    }
  }

  if (toolUse.toolName === 'web_page_navigator') {
    toolText = <>Navigating to the URL: <span className={styles.toolUrl}>{input?.website_url}</span></>
  }

  if (toolUse.toolName === 'web_page_history_navigator') {
    toolText = <>Pressing the browser's <i>{input?.back ? 'back' : 'forwards'}</i> button</>
  }

  if (toolUse.toolName === 'assistant_detail_storage') {
    toolText = (
    <Tooltip>
      Sending information from the screenshot for later
      <div slot='content'>
        <div className={styles.toolUseContentPreview}>
          {input?.information}
        </div>
      </div>
    </Tooltip>
    )
  }

  if (toolUse.toolName === 'user_choice_tool') {
    if (toolUse.output) {
      toolText = (
        <SuggestionButtonRaw
          onClick={() => {}}
          isDisabled={true}
          icon={<Icon className={styles.completedChoiceIcon} name='checkbox-checked' />}
        >
          {toolUse.output[0]?.textContentBlock?.text}
        </SuggestionButtonRaw>
      )
    } else {
      progressIcon = <Icon name='help-outline' />

      const handleChoice = (choice: string) => {
        context.conversationHandler?.respondToToolUseRequest(toolUse.toolId,
            [
              {textContentBlock: { text: choice } } as Mojom.ContentBlock
            ]
        )
      }

      toolText = (
        <>
          {input?.choices?.map((choice: string, i: number) => (
            <div key={i} className={styles.toolChoice}>
              <SuggestionButtonRaw
                className={classnames(styles.choice)}
                isDisabled={false}
                onClick={() => handleChoice(choice)}
                icon={<div className={classnames(styles.choiceNumber)}>{i+1}</div>}
              >
                {choice}
              </SuggestionButtonRaw>
            </div>
          ))}
        </>
      )
    }
  }

  if (toolUse.toolName === 'active_web_page_content_fetcher') {
    progressIcon = <Icon name='help-outline' />
    if (toolUse.output) {
      if (toolUse.output[0]?.textContentBlock?.text?.startsWith('Error')) {
        statusIcon = <Icon className={styles.deniedChoiceIcon} name='remove-circle-outline' />
      }
      toolText = (
        <Tooltip>
          <>Requested page content</>
          <div slot='content'>
          {toolUse.output.map((content, i) => (
            <div key={i} className={styles.toolUseContentPreview}>
              {content.textContentBlock?.text}
            </div>
          ))}
          </div>
        </Tooltip>
      )
    } else {
      const respond = (canSend: boolean) =>
        context.conversationHandler?.respondToToolUseRequest(toolUse.toolId, canSend ? null : [{textContentBlock: { text: 'Error - the user wishes to generate a response without access to the page content.' } } as Mojom.ContentBlock])

      toolText = (<>
        <>Leo is requesting access to page content. Do you want to attach the current page?</>
        <SuggestionButtonRaw
          className={classnames(styles.choice)}
          onClick={() => respond(true)}
          icon={<Icon className={styles.completedChoiceIcon} name='check-normal' />}
        >
          Attach this page's content to the conversation
        </SuggestionButtonRaw>
        <SuggestionButtonRaw
          className={classnames(styles.choice)}
          onClick={() => respond(false)}
          icon={<Icon className={styles.deniedChoiceIcon} name='remove-circle-outline' />}
        >
          Deny
        </SuggestionButtonRaw>
      </>)
    }
  }

  const isComplete = !!toolUse.output
  return (
    <div className={classnames(styles.toolUse, isComplete && styles.toolUseComplete, `tool-${toolUse.toolName}`)}>
      <div className={styles.toolUseIcon} title={toolUse.inputJson}>
        {!isComplete && progressIcon}
        {isComplete && statusIcon}
      </div>
      {toolText}
    </div>
  )
}
