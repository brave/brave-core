/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from 'classnames'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'
import { ConversationTurn, CharacterType } from '../../api/page_handler'

interface ConversationListProps {
  list: ConversationTurn[]
  suggestedQuestions: string[]
  isLoading: boolean
  canGenerateQuestions: boolean
  userAllowsAutoGenerating: boolean
  onSetUserAllowsAutoGenerating: (value: boolean) => void
  onQuestionSubmit: (question: string) => void
  onGenerateSuggestedQuestions: () => void
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
    <div className={styles.list}>
      {props.list.map((turn, id) => {
        const isLastEntry = (id === (props.list.length - 1))
        const isLoading = isLastEntry && props.isLoading
        const elementRef = isLastEntry
          ? lastConversationEntryElementRef
          : null

        const isHuman = turn.characterType === CharacterType.HUMAN
        const isAIAssistant = turn.characterType === CharacterType.ASSISTANT

        const turnClass = classnames({
          [styles.turnAI]: isAIAssistant,
          [styles.turnHuman]: isHuman,
        })

        const avatarStyles = classnames({
          [styles.avatarAI]: isAIAssistant,
          [styles.avatarHuman]: isHuman
        })

        return (
          <div key={id} ref={elementRef} className={turnClass}>
            <div className={avatarStyles}>
              <Icon name={isHuman ? 'user-circle' : 'product-brave-ai'} />
            </div>
            <div className={styles.message}>
              {turn.text}
              {isLoading && <span className={styles.caret}/>}
            </div>
          </div>
        )
      })}
      <div className={styles.suggestedQuestions}>
        {props.suggestedQuestions.map(question => (
          <Button size='small' kind='outline' onClick={() => props.onQuestionSubmit(question)}>
            {question}
          </Button>
        ))}
        {props.canGenerateQuestions && (
          <>
            <Button size='medium' kind='plain' onClick={props.onGenerateSuggestedQuestions}>
              Suggest some questions for this page
            </Button>
          </>
        )}
        <Checkbox size='small' checked={props.userAllowsAutoGenerating} onChanged={(e) => props.onSetUserAllowsAutoGenerating(e.detail.checked)}>
          Automatically suggest questions when I visit a page
        </Checkbox>
      </div>
    </div>
  )
}

export default ConversationList
