/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from 'classnames'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import { ConversationTurn, CharacterType } from '../../api/page_handler'
import { getLocale } from '$web-common/locale'

interface ConversationListProps {
  list: ConversationTurn[]
  suggestedQuestions: string[]
  isLoading: boolean
  onQuestionSubmit: (question: string) => void
}

const elementScrollBehavior: ScrollIntoViewOptions = {
  behavior: 'smooth',
  block: 'start'
}

function ConversationList (props: ConversationListProps) {
  // Scroll the last conversation item in to view when entries are added.
  const lastConversationEntryElementRef = React.useRef<HTMLDivElement>(null)
  const loadingElementRef = React.useRef<HTMLDivElement>(null)
  React.useEffect(() => {
    if (!props.list.length && !props.isLoading) {
      return
    }
    if (props.isLoading) {
      if (!loadingElementRef.current) {
        console.error('Conversation loading element did not exist when expected')
      } else {
        loadingElementRef.current.scrollIntoView(elementScrollBehavior)
      }
      return
    }
    if (!lastConversationEntryElementRef.current) {
      console.error('Conversation entry element did not exist when expected')
    } else {
      lastConversationEntryElementRef.current.scrollIntoView(elementScrollBehavior)
    }
  }, [props.list.length, props.isLoading])

  return (
    <div className={styles.list}>
      {props.list.map((turn, id) => {
        const turnClass = classnames({
          [styles.turnAI]: turn.characterType === CharacterType.ASSISTANT,
          [styles.turnHuman]: turn.characterType === CharacterType.HUMAN,
        })

        const isLastEntry = (id === (props.list.length - 1))
        const elementRef = isLastEntry
          ? lastConversationEntryElementRef
          : null

        return (
          <div key={id} ref={elementRef} className={turnClass}>
            <p>
              {turn.text}
            </p>
          </div>
        )
      })}
      {props.isLoading && (
        <div className={styles.turnAI} ref={loadingElementRef}>
          <p>
            {getLocale('loadingLabel')}
            <span className={styles.caret}/>
          </p>
        </div>
      )}
      <div className={styles.suggestedQuestions}>
        {props.suggestedQuestions.map(question => (
          <Button size='small' kind='outline' onClick={() => props.onQuestionSubmit(question)}>
            {question}
          </Button>
        ))}
      </div>
    </div>
  )
}

export default ConversationList
