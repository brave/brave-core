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
import Quote from '../quote'

const CodeBlock = React.lazy(async () => ({ default: (await import('../code_block')).default.Block }))
const CodeInline = React.lazy(async () => ({ default: (await import('../code_block')).default.Inline }))

// Capture markdown-style code blocks and inline code.
// It captures:
// 1. Multiline code blocks with optional language specifiers (```lang\n...code...```).
// 2. Inline code segments (`code`).
// 3. Regular text outside of code segments.
const codeFormatRegexp = /```([^\n`]+)?\n?([\s\S]*?)```|`(.*?)`|([^`]+)/gs

const SUGGESTION_STATUS_SHOW_BUTTON: mojom.SuggestionGenerationStatus[] = [
  mojom.SuggestionGenerationStatus.CanGenerate,
  mojom.SuggestionGenerationStatus.IsGenerating
]

interface ConversationListProps {
  onLastElementHeightChange: () => void
}

interface FormattedTextProps {
  text: string
}

function FormattedTextRenderer(props: FormattedTextProps): JSX.Element {
  const nodes = React.useMemo(() => {
    const formattedNodes = Array.from(props.text.matchAll(codeFormatRegexp)).map((match: any) => {
      if (match[0].substring(0,3).includes('```')) {
        return (<React.Suspense fallback={'...'}>
          <CodeBlock lang={match[1]} code={match[2].trim()} />
        </React.Suspense>)
      } else if (match[0].substring(0,1).includes('`')) {
        return (
          <React.Suspense fallback={'...'}>
            <CodeInline code={match[3]}/>
        </React.Suspense>
        )
      } else {
        return match[0]
      }
    })

    return <>{formattedNodes}</>
  }, [props.text])

  return nodes
}

function ConversationList(props: ConversationListProps) {
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

  const handleQuestionSubmit = (question: string) => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(question)
  }

  const lastEntryElementRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (!lastEntryElementRef.current) return
    props.onLastElementHeightChange()
  }, [conversationHistory.length, lastEntryElementRef.current?.clientHeight])

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
              ref={isLastEntry ? lastEntryElementRef : null}
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
                <div
                  className={styles.message}
                >
                  {<FormattedTextRenderer text={turn.text} />}
                  {isLoading && <span className={styles.caret} />}
                  {turn.selectedText && <Quote text={turn.selectedText} />}
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
