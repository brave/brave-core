/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import AlertCenter from '@brave/leo/react/alertCenter'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { useConversation } from '../../state/conversation_context'
import { useAIChat } from '../../state/ai_chat_context'
import { isLeoModel } from '../../model_utils'
import ErrorConnection from '../alerts/error_connection'
import ErrorConversationEnd from '../alerts/error_conversation_end'
import ErrorInvalidEndpointURL from '../alerts/error_invalid_endpoint_url'
import ErrorRateLimit from '../alerts/error_rate_limit'
import LongConversationInfo from '../alerts/long_conversation_info'
import NoticeConversationStorage from '../notices/notice_conversation_storage'
import WarningPremiumDisconnected from '../alerts/warning_premium_disconnected'
import ConversationsList from '../conversations_list'
import { ConversationHeader } from '../header'
import InputBox from '../input_box'
import ModelIntro from '../model_intro'
import PageContextToggle from '../page_context_toggle'
import PremiumSuggestion from '../premium_suggestion'
import PrivacyMessage from '../privacy_message'
import { SuggestedQuestion, SuggestionButton } from '../suggested_question'
import ToolsButtonMenu from '../tools_button_menu'
import WelcomeGuide from '../welcome_guide'
import styles from './style.module.scss'
import SiteTitle from '../site_title'

const SCROLL_BOTTOM_THRESHOLD = 10.0

const SUGGESTION_STATUS_SHOW_BUTTON: Mojom.SuggestionGenerationStatus[] = [
  Mojom.SuggestionGenerationStatus.CanGenerate,
  Mojom.SuggestionGenerationStatus.IsGenerating
]

function Main() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const [isConversationListOpen, setIsConversationsListOpen] = React.useState(false)
  const [isContentReady, setIsContentReady] = React.useState(false)

  const shouldShowPremiumSuggestionForModel =
    aiChatContext.hasAcceptedAgreement &&
    !aiChatContext.isPremiumStatusFetching && // Avoid flash of content
    !aiChatContext.isPremiumUser &&
    conversationContext.currentModel?.options.leoModelOptions?.access === Mojom.ModelAccess.PREMIUM

  const shouldShowPremiumSuggestionStandalone =
    aiChatContext.hasAcceptedAgreement &&
    !aiChatContext.isPremiumStatusFetching && // Avoid flash of content
    !shouldShowPremiumSuggestionForModel && // Don't show 2 premium prompts
    !conversationContext.apiHasError && // Don't show premium prompt and errors (rate limit error has its own premium prompt suggestion)
    !aiChatContext.isStorageNoticeDismissed && // Don't show premium prompt and storage notice
    aiChatContext.canShowPremiumPrompt &&
    conversationContext.associatedContentInfo === null && // SiteInfo request has finished and this is a standalone conversation
    !aiChatContext.isPremiumUser


  const isLastTurnBraveSearchSERPSummary =
    conversationContext.conversationHistory.at(-1)?.fromBraveSearchSERP ?? false

  const showContextToggle =
    (conversationContext.conversationHistory.length === 0 ||
      isLastTurnBraveSearchSERPSummary) &&
    conversationContext.associatedContentInfo?.isContentAssociationPossible

  let currentErrorElement = null

  let scrollerElement: HTMLDivElement | null = null
  const headerElement = React.useRef<HTMLDivElement>(null)
  const conversationContentElement = React.useRef<HTMLDivElement>(null)

  const scrollPos = React.useRef({ isAtBottom: true })

  // Determine which, if any, error message should be displayed
  if (aiChatContext.hasAcceptedAgreement && conversationContext.apiHasError) {
    switch (conversationContext.currentError) {
      case Mojom.APIError.ConnectionIssue:
        currentErrorElement = <ErrorConnection
          onRetry={conversationContext.retryAPIRequest} />
        break
      case Mojom.APIError.RateLimitReached:
        currentErrorElement = <ErrorRateLimit />
        break
      case Mojom.APIError.ContextLimitReached:
        currentErrorElement = <ErrorConversationEnd />
        break
      case Mojom.APIError.InvalidEndpointURL:
        currentErrorElement = <ErrorInvalidEndpointURL />
        break
    }
  }

  const handleScroll = (e: React.UIEvent<HTMLDivElement>) => {
    // Monitor scroll positions only when Assistant is generating
    if (!conversationContext.isGenerating) return
    const el = e.currentTarget
    scrollPos.current.isAtBottom = Math.abs(el.scrollHeight - el.clientHeight - el.scrollTop) < SCROLL_BOTTOM_THRESHOLD
  }

  const handleLastElementHeightChange = () => {
    if (!scrollerElement) {
      return
    }

    if (scrollPos.current.isAtBottom) {
      scrollerElement.scrollTop = scrollerElement.scrollHeight - scrollerElement.clientHeight
    }
  }

  // Ask for opt-in once the first message is sent
  const showAgreementModal = !aiChatContext.hasAcceptedAgreement &&
    !!conversationContext.conversationHistory.length

  const showSuggestions: boolean =
    aiChatContext.hasAcceptedAgreement &&
    (conversationContext.suggestedQuestions.length > 0 ||
      SUGGESTION_STATUS_SHOW_BUTTON.includes(conversationContext.suggestionStatus))

  const viewPortWithoutKeyboard = React.useRef(0)
  const keyboardSize = React.useRef(0)

  React.useEffect(() => {
    const handler = () => {
      if (!aiChatContext.isMobile || !window.visualViewport) {
        return
      }
      const viewPortWithKeyboard = window.visualViewport.height
      if (!headerElement.current || !conversationContentElement.current ||
        viewPortWithKeyboard === 0 || viewPortWithoutKeyboard.current === 0) {
        return
      }
      if (keyboardSize.current === 0 ||
        keyboardSize.current <
        viewPortWithoutKeyboard.current - viewPortWithKeyboard) {
        keyboardSize.current =
          viewPortWithoutKeyboard.current - viewPortWithKeyboard
      }
      const mountPoint = document.getElementById('mountPoint')
      if (mountPoint) {
        if (mountPoint.clientHeight >=
          (headerElement.current.clientHeight +
            conversationContentElement.current.clientHeight) * 2) {
          const percent = viewPortWithKeyboard * 100 /
            viewPortWithoutKeyboard.current
          mountPoint.style.height = `${percent}%`
        } else if (keyboardSize.current >
          viewPortWithoutKeyboard.current - viewPortWithKeyboard) {
          mountPoint.style.height = '100%'
        }
      }
    }
    window.addEventListener('resize', handler)
    return () => {
      window.removeEventListener('resize', handler)
    }
  }, [])

  const handleOnFocusInputMobile = () => {
    if (window.visualViewport != null) {
      viewPortWithoutKeyboard.current = window.visualViewport.height
    }
  }

  return (
    <main className={styles.main}>
      {isConversationListOpen && !aiChatContext.isStandalone && (
        <div className={styles.conversationsList}>
          <div
        className={classnames({
          [styles.conversationsListHeader]: true,
        })}
      >
        <Button
          kind='plain-faint'
          fab
          onClick={() => {
            setIsConversationsListOpen?.(false)
          }}
        >
          <Icon name='arrow-left' />
        </Button>
      </div>
          <ConversationsList
            setIsConversationsListOpen={setIsConversationsListOpen}
          />
        </div>
      )}
      {showAgreementModal && <PrivacyMessage />}
      <ConversationHeader ref={headerElement}
        setIsConversationsListOpen={setIsConversationsListOpen}
      />
      <div className={classnames({
        [styles.scroller]: true,
        [styles.flushBottom]: !aiChatContext.hasAcceptedAgreement
      })}
        ref={node => (scrollerElement = node)}
        onScroll={handleScroll}
      >
        <AlertCenter position='top-left' className={styles.alertCenter} />
        <div
          className={classnames({
            [styles.conversationContent]: true,
            [styles.contentReady]: !conversationContext.conversationUuid || isContentReady
          })}
          ref={conversationContentElement}
        >
          {aiChatContext.hasAcceptedAgreement && (
            <>
              <ModelIntro />
              {conversationContext.associatedContentInfo?.isContentAssociationPossible && conversationContext.shouldSendPageContents && (
                    <div className={styles.siteTitleContainer}>
                      <SiteTitle size='default' />
                    </div>
              )}
              {!!conversationContext.conversationUuid &&
              <aiChatContext.conversationEntriesComponent
                onLoad={() => setIsContentReady(true)}
                onGeneratedConversationEntryHeightChanged={handleLastElementHeightChange}
              />
              }
              {showSuggestions && (
              <div className={styles.suggestedQuestionsBox}>
                <div className={styles.questionsList}>
                  {conversationContext.suggestedQuestions.map((question, i) => <SuggestedQuestion key={question} question={question} />)}
                  {SUGGESTION_STATUS_SHOW_BUTTON.includes(
                    conversationContext.suggestionStatus
                  ) && conversationContext.shouldSendPageContents && (
                      <SuggestionButton
                        onClick={() => conversationContext.generateSuggestedQuestions()}
                        isLoading={conversationContext.suggestionStatus === Mojom.SuggestionGenerationStatus.IsGenerating}
                      >
                        <span className={styles.buttonText}>
                          {getLocale('suggestQuestionsLabel')}
                        </span>
                      </SuggestionButton>
                    )}
                </div>
              </div>
            )}
            </>
          )}
          {currentErrorElement && (
            <div className={styles.promptContainer}>{currentErrorElement}</div>
          )}
          {aiChatContext.hasAcceptedAgreement && !aiChatContext.isStorageNoticeDismissed && (
            <div className={styles.promptContainer}>
              <NoticeConversationStorage />
            </div>
          )}
          {shouldShowPremiumSuggestionForModel && (
            <div className={styles.promptContainer}>
              <PremiumSuggestion
                title={getLocale('unlockPremiumTitle')}
                secondaryActionButton={
                  <Button
                    kind='plain-faint'
                    onClick={() => conversationContext.switchToBasicModel()}
                  >
                    {getLocale('switchToBasicModelButtonLabel')}
                  </Button>
                }
              />
            </div>
          )}
          {shouldShowPremiumSuggestionStandalone && (
            <div className={styles.promptContainer}>
              <PremiumSuggestion
                title={getLocale('unlockPremiumTitle')}
                secondaryActionButton={
                  <Button
                    kind='plain-faint'
                    onClick={() => aiChatContext.dismissPremiumPrompt()}
                  >
                    {getLocale('dismissButtonLabel')}
                  </Button>
                }
              />
            </div>
          )}
          {aiChatContext.isPremiumUserDisconnected && (!conversationContext.currentModel || isLeoModel(conversationContext.currentModel)) && (
            <div className={styles.promptContainer}>
              <WarningPremiumDisconnected />
            </div>
          )}
          {conversationContext.shouldShowLongConversationInfo && (
            <div className={styles.promptContainer}>
              <LongConversationInfo />
            </div>
          )}
          {!aiChatContext.hasAcceptedAgreement && !conversationContext.conversationHistory.length && (
            <WelcomeGuide />
          )}
        </div>
      </div>
      <div className={styles.input}>
        {showContextToggle && (
          <div className={styles.toggleContainer}>
            <PageContextToggle />
          </div>
        )}
        <ToolsButtonMenu {...conversationContext}>
          <InputBox
            context={{ ...conversationContext, ...aiChatContext }}
            onFocusInputMobile={handleOnFocusInputMobile}
          />
        </ToolsButtonMenu>
      </div>
    </main>
  )
}

export default Main
