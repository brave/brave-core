// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import { createTextContentBlock } from '../../../common/content_block'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event.module.scss'

// Matches the choice_type input property values of the user choice tool. Any
// other value, including a missing one, is treated as a preference choice.
const FOLLOW_UP_CHOICE_TYPE = 'follow_up'

const ToolEventContentUserChoice: ToolComponent = (props) => {
  const context = useUntrustedConversationContext()
  const content: ToolUseContent = {
    toolLabel: null,
    expandedContent: null,
  }

  // Follow-ups are questions the user could ask next, not a decision the
  // assistant is waiting on. The browser moves them to the conversation's
  // suggested questions, so nothing is displayed with the response itself.
  if (props.toolInput?.choice_type === FOLLOW_UP_CHOICE_TYPE) {
    return props.children(content)
  }

  if (props.toolUseEvent.output) {
    // Tool already completed, don't allow further response
    content.expandedContent = (
      <ConversationAreaButton
        isDisabled
        icon={
          <Icon
            className={styles.completedChoiceIcon}
            name='checkbox-checked'
          />
        }
      >
        <span data-testid='tool-choice-text-0'>
          {props.toolUseEvent.output[0]?.textContentBlock?.text}
        </span>
      </ConversationAreaButton>
    )
  } else if (props.toolInput?.choices?.length) {
    const handleChoice = (choice: string) => {
      if (!props.isEntryActive) {
        return
      }
      context.conversationHandler?.respondToToolUseRequest(
        props.toolUseEvent.id,
        [createTextContentBlock(choice)],
        [],
      )
    }

    content.expandedContent = (
      <>
        {props.toolInput?.choices?.map((choice: string, i: number) => (
          <div
            key={i}
            className={styles.toolChoice}
          >
            <ConversationAreaButton
              className={classnames(styles.choice)}
              isDisabled={!props.isEntryActive}
              onClick={() => handleChoice(choice)}
              icon={
                <div className={classnames(styles.choiceNumber)}>{i + 1}</div>
              }
            >
              <span data-testid={`tool-choice-text-${i}`}>{choice}</span>
            </ConversationAreaButton>
          </div>
        ))}
      </>
    )
  }

  return props.children(content)
}

export default ToolEventContentUserChoice
