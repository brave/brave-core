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
import { IconName } from '@brave/leo/icons/meta'

export function useToolEventContent(toolUseEvent: Mojom.ToolUseEvent) {
  const context = useUntrustedConversationContext()

  let iconName: IconName | null = null

  const input = React.useMemo(() => {
    if (!toolUseEvent?.inputJson) {
      return null
    }
    try {
      return JSON.parse(toolUseEvent.inputJson)
    } catch (e) {
      return null
    }
  }, [toolUseEvent?.inputJson])

  let toolText = <>{toolUseEvent.toolName}</>
  let tooltipContent: JSX.Element | null = null
  let statusIcon = <Icon name="check-circle-outline" />
  let progressIcon = <ProgressRing />

  if (toolUseEvent.toolName === 'computer') {
    switch (input?.action) {
      case 'screenshot': {
        iconName = 'screenshot'
        toolText = <>Looking at the page</>
        break
      }
      case 'key': {
        iconName = 'keyboard'
        toolText = <>Pressing the key: {input?.text}</>
        break
      }
      case 'type': {
        iconName = 'edit-box'
        toolText = <div title={input?.text} className={styles.toolUseLongText}>Typing: "{input?.text}"</div>
        break
      }
      case 'mouse_move': {
        iconName = 'mouse-two-buttons'
        toolText = <>Moving the mouse</>
        break
      }
      case 'left_click': {
        iconName = 'mouse-two-buttons'
        toolText = <>Clicking the left mouse button</>
        break
      }
      case 'scroll': {
        iconName = 'mouse-scroll'
        toolText = <>Scrolling</>
        break
      }
      default: {
        toolText = <>{input?.action}</>
      }
    }

    // Append any images (e.g. screenshots) in the output as a tooltip
    const toolResultOutputImageContentBlocks = toolUseEvent.output?.filter(content => !!content.imageContentBlock) ?? []

    if (toolResultOutputImageContentBlocks.length) {
      tooltipContent = <>
        {toolResultOutputImageContentBlocks.map((content, i) => (
          <div key={i}>
            <img className={styles.toolUseContentPreview} src={content.imageContentBlock?.imageUrl} />
          </div>
        ))}
      </>
    }
  }

  if (toolUseEvent.toolName === 'web_page_navigator') {
    iconName = 'internet'
    toolText = <>Navigating to the URL: <span className={styles.toolUrl}>{input?.website_url}</span></>
  }

  if (toolUseEvent.toolName === 'web_page_history_navigator') {
    iconName = 'internet'
    toolText = <>Pressing the browser's <i>{input?.back ? 'back' : 'forwards'}</i> button</>
  }

  if (toolUseEvent.toolName === 'assistant_detail_storage') {
    iconName = 'database'
    toolText = <>Sending information from the screenshot for later</>
    tooltipContent = (
        <div className={styles.toolUseContentPreview}>
          {input?.information}
        </div>
    )
  }

  if (toolUseEvent.toolName === 'user_choice_tool') {
    if (toolUseEvent.output) {
      toolText = (
        <SuggestionButtonRaw
          onClick={() => {}}
          isDisabled={true}
          icon={<Icon className={styles.completedChoiceIcon} name='checkbox-checked' />}
        >
          {toolUseEvent.output[0]?.textContentBlock?.text}
        </SuggestionButtonRaw>
      )
    } else {
      progressIcon = <Icon name='help-outline' />

      const handleChoice = (choice: string) => {
        context.conversationHandler?.respondToToolUseRequest(toolUseEvent.toolId,
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

  if (toolUseEvent.toolName === 'active_web_page_content_fetcher') {
    progressIcon = <Icon name='help-outline' />
    if (toolUseEvent.output) {
      if (toolUseEvent.output[0]?.textContentBlock?.text?.startsWith('Error')) {
        statusIcon = <Icon className={styles.deniedChoiceIcon} name='remove-circle-outline' />
      }
      toolText = <>Requested page content</>
      tooltipContent = (<>
        {toolUseEvent.output.map((content, i) => (
          <div key={i} className={styles.toolUseContentPreview}>
            {content.textContentBlock?.text}
          </div>
        ))}
      </>)
    } else {
      const respond = (canSend: boolean) =>
        context.conversationHandler?.respondToToolUseRequest(toolUseEvent.toolId, canSend ? null : [{textContentBlock: { text: 'Error - the user wishes to generate a response without access to the page content.' } } as Mojom.ContentBlock])

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

  return {
    iconName,
    input,
    progressIcon,
    statusIcon,
    tooltipContent,
    toolText
  }
}


export default function ToolEvent(props: { event: Mojom.ToolUseEvent, isActiveEntry: boolean }) {
  const toolUse = props.event

  const {
    progressIcon,
    statusIcon,
    toolText,
    tooltipContent
  } = useToolEventContent(toolUse)

  const isComplete = !!toolUse.output
  return (
    <div className={classnames(styles.toolUse, isComplete && styles.toolUseComplete, `tool-${toolUse.toolName}`)}>
      <div className={styles.toolUseIcon} title={toolUse.inputJson}>
        {!isComplete && progressIcon}
        {isComplete && statusIcon}
      </div>
      {tooltipContent
      ? (
      <Tooltip>
        {toolText}
        <div slot='content'>
          {tooltipContent}
        </div>
      </Tooltip>
      )
      : toolText
      }
    </div>
  )
}
