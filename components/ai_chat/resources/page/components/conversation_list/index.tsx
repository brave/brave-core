/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from 'classnames'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'
import { ConversationTurn, CharacterType, ConversationTurnStatus }
  from '../../api/page_handler'
import { RetryConversationItem } from './retry_conversation_item'

interface ConversationListProps {
  list: ConversationTurn[]
  suggestedQuestions: string[]
  isLoading: boolean
  onQuestionSubmit: (question: string) => void
  onRetrySubmit: (uuid: string) => void
}

function ConversationList (props: ConversationListProps) {
  // Scroll the last conversation item in to view when entries are added.
  const lastConversationEntryElementRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (!props.list.length && !props.isLoading) {
      return
    }

    if (!lastConversationEntryElementRef.current) {
      console.error('Conversation entry element did not exist when expected')
    } else {
      lastConversationEntryElementRef.current.scrollIntoView(false)
    }
  }, [props.list.length, props.isLoading, lastConversationEntryElementRef.current?.clientHeight])

  return (
    <>
      <div>
      {props.list.map((turn, id) => {
        const prevUuid = props.list[id - 1] && props.list[id - 1].uuid ? props.list[id - 1].uuid : '0';
        const isLastEntry = (id === (props.list.length - 1))
        const isLoading = isLastEntry && props.isLoading
        const elementRef = isLastEntry
          ? lastConversationEntryElementRef
          : null

        const isHuman = turn.characterType === CharacterType.HUMAN
        const isAIAssistant = turn.characterType === CharacterType.ASSISTANT
        const isError = turn.status === ConversationTurnStatus.ABNORMAL

        const turnClass = classnames({
          [styles.turn]: true,
          [styles.turnAI]: isAIAssistant,
        })

        const avatarStyles = classnames({
          [styles.avatar]: true,
          [styles.avatarAI]: isAIAssistant,
        })

        if(isError) {
          return <>
          <RetryConversationItem
                    id={id} 
                    prevItemUuid= {prevUuid}
                    turn={turn}
                    isLoading = {isLoading}
                    elementRef={elementRef}
                    onRetrySubmit={props.onRetrySubmit}
                    />
          </>
        } else {
          return (
            <div key={id} ref={elementRef} className={turnClass}>
              <div className={avatarStyles}>
                  <Icon name={isHuman ? 'user-circle' : 'product-brave-ai'} />
              </div>
              <div className={styles.message}>
                <div className={styles.messageTextBox}>
                  {turn.text}
                  {isLoading && <span className={styles.caret}/>}
                </div>
              </div>
            </div>
          )
        }
      })}
      </div>
      {props.suggestedQuestions.length > 0 && (
        <div className={styles.suggestedQuestionsBox}>
          <div className={styles.suggestedQuestionLabel}>
            <Icon name="product-brave-ai" />
            <div>Suggested follow-ups</div>
          </div>
          <div className={styles.questionsList}>
            {props.suggestedQuestions.map((question, id) => (
              <Button key={id} kind='outline' onClick={() => props.onQuestionSubmit(question)}>
                <span className={styles.buttonText}>
                  {question}
                </span>
              </Button>
            ))}
          </div>
        </div>
      )}
    </>
  )
}

export default ConversationList
