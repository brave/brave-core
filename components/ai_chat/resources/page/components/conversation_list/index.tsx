/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import useLongPress from '$web-common/useLongPress'

import styles from './style.module.scss'
import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import ContextMenuAssistant from '../context_menu_assistant'
import { getLocale } from '$web-common/locale'
import SiteTitle from '../site_title'
import Quote from '../quote'
import ActionTypeLabel from '../action_type_label'
import LongPageInfo from '../alerts/long_page_info'
import MarkdownRenderer from '../markdown_renderer'

const SUGGESTION_STATUS_SHOW_BUTTON: mojom.SuggestionGenerationStatus[] = [
  mojom.SuggestionGenerationStatus.CanGenerate,
  mojom.SuggestionGenerationStatus.IsGenerating
]

interface ConversationListProps {
  onLastElementHeightChange: () => void
}

function ConversationList(props: ConversationListProps) {
  const context = React.useContext(DataContext)
  const {
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

  const handleQuestionSubmit = (question: string) => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(question)
  }

  const lastEntryElementRef = React.useRef<HTMLDivElement>(null)
  const [activeMenuId, setActiveMenuId] = React.useState<number | null>()

  const showAssistantMenu = (id: number) => {
    setActiveMenuId(id)
  }

  const hideAssistantMenu = () => {
    setActiveMenuId(null)
  }

  const longPressProps = useLongPress({
    onLongPress: (e: React.TouchEvent) => {
      const currentTarget = e.currentTarget as HTMLElement
      const id = currentTarget.getAttribute('data-id')
      if (id === null) return
      showAssistantMenu(parseInt(id))
    },
    onTouchMove: () => setActiveMenuId(null)
  })

  React.useEffect(() => {
    if (lastEntryElementRef.current === null) return
    if (!context.isGenerating) return
    props.onLastElementHeightChange()
  }, [conversationHistory.length, lastEntryElementRef.current?.clientHeight])

  return (
    <>
      <div>
        {conversationHistory.map((turn, id) => {
          const isLastEntry = id === conversationHistory.length - 1
          const shouldShowTextCursor = isLastEntry && context.isGenerating
          const isHuman = turn.characterType === mojom.CharacterType.HUMAN
          const isAIAssistant = turn.characterType === mojom.CharacterType.ASSISTANT
          const showSiteTitle = id === 0 && isHuman && shouldSendPageContents
          const showLongPageContentInfo = id === 1 && isAIAssistant && context.shouldShowLongPageWarning

          const turnContainer = classnames({
            [styles.turnContainerMobile]: context.isMobile,
            [styles.turnContainerHighlight]: isAIAssistant && activeMenuId === id
          })

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant,
          })

          const avatarStyles = classnames({
            [styles.avatar]: true,
            [styles.avatarAI]: isAIAssistant
          })

          return (
            <div
              key={id}
              className={turnContainer}
              ref={isLastEntry ? lastEntryElementRef : null}
            >
              <div
                data-id={id}
                className={turnClass}
                onMouseLeave={isAIAssistant ? () => setActiveMenuId(null) : undefined}
                {...(isAIAssistant ? longPressProps : {})}
              >
                {isAIAssistant && (
                  <div className={styles.asistantMenu}>
                    <ContextMenuAssistant
                      ref={portalRefs}
                      turnId={id}
                      turnText={turn.text}
                      isOpen={activeMenuId === id}
                      onClick={() => showAssistantMenu(id)}
                      onClose={hideAssistantMenu}
                    />
                  </div>
                )}
                <div className={avatarStyles}>
                  <Icon name={isHuman ? 'user-circle' : 'product-brave-leo'} />
                </div>
                <div
                  className={styles.message}
                >
                  {
                    !turn.selectedText &&
                      (isAIAssistant ? (
                        <MarkdownRenderer
                          text={turn.text}
                          shouldShowTextCursor={shouldShowTextCursor}
                        />
                      ) : (turn.text))
                  }
                  {turn.selectedText &&
                      <ActionTypeLabel actionType={turn.actionType} />}
                  {turn.selectedText && <Quote text={turn.selectedText} />}
                  {showSiteTitle && <div className={styles.siteTitleContainer}><SiteTitle size="default" /></div>}
                  {showLongPageContentInfo && <LongPageInfo />}
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
