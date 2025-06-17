// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import { createTextContentBlock } from '../../../common/content_block'
import * as Mojom from '../../../common/mojom'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './tool_event.module.scss'

interface Props {
  toolUseEvent: Mojom.ToolUseEvent
  isEntryActive: boolean
}

// Content customizable for each known tool type
interface ToolUseContent {
  toolText: JSX.Element
  tooltipContent: JSX.Element | null
  statusIcon: JSX.Element
  progressIcon: JSX.Element
}

function useUserChoiceToolContent(props: Props, toolInput: any): Partial<ToolUseContent> {
  const context = useUntrustedConversationContext()
  const content: Partial<ToolUseContent> = {}

  if (props.toolUseEvent.output) {
    // Tool already completed, don't allow further response
    content.toolText = (
      <ConversationAreaButton
        onClick={() => {}}
        isDisabled={true}
        icon={<Icon className={styles.completedChoiceIcon} name='checkbox-checked' />}
      >
        {props.toolUseEvent.output[0]?.textContentBlock?.text}
      </ConversationAreaButton>
    )
  } else {
    content.progressIcon = <Icon name='help-outline' />

    const handleChoice = (choice: string) => {
      if (!props.isEntryActive) {
        return
      }
      context.conversationHandler?.respondToToolUseRequest(
        props.toolUseEvent.id, [createTextContentBlock(choice)]
      )
    }

    content.toolText = (
      <>
        {toolInput?.choices?.map((choice: string, i: number) => (
          <div key={i} className={styles.toolChoice}>
            <ConversationAreaButton
              className={classnames(styles.choice)}
              isDisabled={!props.isEntryActive}
              onClick={() => handleChoice(choice)}
              icon={<div className={classnames(styles.choiceNumber)}>{i+1}</div>}
            >
              {choice}
            </ConversationAreaButton>
          </div>
        ))}
      </>
    )
  }

  return content
}

export function useToolEventContent(props: Props): ToolUseContent {
  const {toolUseEvent} = props

  let content: ToolUseContent = {
    toolText: <>{toolUseEvent.toolName}</>,
    tooltipContent: null,
    statusIcon: <Icon name="check-circle-outline" />,
    progressIcon: <ProgressRing />
  }

  const input = React.useMemo(() => {
    if (!toolUseEvent?.argumentsJson) {
      return null
    }
    try {
      return JSON.parse(toolUseEvent.argumentsJson)
    } catch (e) {
      return null
    }
  }, [toolUseEvent?.argumentsJson])

  if (toolUseEvent.toolName === Mojom.USER_CHOICE_TOOL_NAME) {
    content = { ...content, ...useUserChoiceToolContent(props, input)}
  }

  return content
}


export default function ToolEvent(props: Props) {
  const {
    progressIcon,
    statusIcon,
    toolText,
    tooltipContent
  } = useToolEventContent(props)

  const isComplete = !!props.toolUseEvent.output
  return (
    <div className={classnames(styles.toolUse, isComplete && styles.toolUseComplete, `tool-${props.toolUseEvent.toolName}`)}>
      <div className={styles.toolUseIcon} title={props.toolUseEvent.argumentsJson}>
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
