/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from 'classnames'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'
import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import ContextMenuAssistant from '../context_menu_assistant'
import { getLocale } from '$web-common/locale'
import SiteTitle from '../site_title'

const SUGGESTION_STATUS_SHOW_BUTTON: mojom.SuggestionGenerationStatus[] = [
  mojom.SuggestionGenerationStatus.CanGenerate,
  mojom.SuggestionGenerationStatus.IsGenerating
]

function ConversationList() {
  // Scroll the last conversation item in to view when entries are added.
  const lastConversationEntryElementRef = React.useRef<HTMLDivElement>(null)

  const context = React.useContext(DataContext)
  const {
    isGenerating,
    conversationHistory,
    suggestedQuestions,
    shouldDisableUserInput,
    hasAcceptedAgreement,
    shouldSendPageContents
  } = context

  const portalRefs = React.useRef<Map<number, Element>>(new Map())

  const showSuggestions: boolean =
    hasAcceptedAgreement && context.shouldSendPageContents && (
    suggestedQuestions.length > 0 ||
    SUGGESTION_STATUS_SHOW_BUTTON.includes(context.suggestionStatus))

  React.useEffect(() => {
    if (!conversationHistory.length && !isGenerating) {
      return
    }

    if (!lastConversationEntryElementRef.current) {
      console.error('Conversation entry element did not exist when expected')
    } else {
      lastConversationEntryElementRef.current.scrollIntoView(false)
    }
  }, [
    conversationHistory.length,
    isGenerating,
    lastConversationEntryElementRef.current?.clientHeight
  ])

  const handleQuestionSubmit = (question: string) => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(question)
  }

  return (
    <>
      <div>
        {conversationHistory.map((turn, id) => {
          const isLastEntry = id === conversationHistory.length - 1
          const isLoading = isLastEntry && isGenerating
          const isHuman = turn.characterType === mojom.CharacterType.HUMAN
          const isAIAssistant = turn.characterType === mojom.CharacterType.ASSISTANT
          const showSiteTitle = id === 0 && isHuman && shouldSendPageContents

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant
          })

          const avatarStyles = classnames({
            [styles.avatar]: true,
            [styles.avatarAI]: isAIAssistant
          })

          return (
            <div
              key={id}
              ref={isLastEntry ? lastConversationEntryElementRef : null}
            >
              <div className={turnClass}>
                {isAIAssistant && (
                  <div className={styles.turnActions}>
                    <ContextMenuAssistant
                      className={styles.turnMenuButton}
                      ref={portalRefs}
                      turnId={id}
                      turnText={turn.text}
                    />
                  </div>
                )}
                <div className={avatarStyles}>
                  <Icon name={isHuman ? 'user-circle' : 'product-brave-leo'} />
                </div>
                <div className={styles.message}>
                  {turn.text}
                  {isLoading && <span className={styles.caret} />}
                  {showSiteTitle && <div className={styles.siteTitleContainer}><SiteTitle size="default" /></div>}
                </div>
              </div>
              {isAIAssistant ? (
                <div
                  ref={(el) => el && portalRefs.current.set(id, el)}
                  className={styles.feedbackFormPortal}
                />
              ) : null}
            </div>
          )
        })}
      </div>
      {showSuggestions && (
        <div className={styles.suggestedQuestionsBox}>
          <div className={styles.suggestedQuestionLabel}>
            {getLocale('suggestionsTitle')}
          </div>
          <div className={styles.questionsList}>
            {suggestedQuestions.map((question, id) => (
              <Button
                key={id}
                kind='outline'
                onClick={() => handleQuestionSubmit(question)}
                isDisabled={shouldDisableUserInput}
              >
                <span className={styles.buttonText}>{question}</span>
              </Button>
            ))}
            {SUGGESTION_STATUS_SHOW_BUTTON.includes(
              context.suggestionStatus
            ) && (
              <Button
                onClick={() => context.generateSuggestedQuestions()}
                // isDisabled={context.suggestionStatus === mojom.SuggestionGenerationStatus.IsGenerating}
                isLoading={
                  context.suggestionStatus ===
                  mojom.SuggestionGenerationStatus.IsGenerating
                }
                kind='outline'
              >
                <span className={styles.buttonText}>
                  {getLocale('suggestQuestionsLabel')}
                </span>
              </Button>
            )}
          </div>
        </div>
      )}
    </>
  )
}

export default ConversationList
