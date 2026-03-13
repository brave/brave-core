// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import scrollerStyles from '../../../common/scroller.module.scss'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import ConversationEntries from '../conversation_entries'
import ModelIntro from '../model_intro'
import TemporaryChatInfo from '../alerts/temporary_chat_info'
import {
  SuggestedQuestion,
  GenerateSuggestionsButton,
} from '../suggested_question'
import ErrorRateLimit from '../alerts/error_rate_limit'
import ErrorConnection from '../alerts/error_connection'
import ErrorConversationEnd from '../alerts/error_conversation_end'
import ErrorInvalidAPIKey from '../alerts/error_invalid_api_key'
import ErrorInvalidEndpointURL from '../alerts/error_invalid_endpoint_url'
import ErrorServiceOverloaded from '../alerts/error_service_overloaded'
import NoticeConversationStorage from '../notices/notice_conversation_storage'
import PremiumSuggestion from '../premium_suggestion'
import WarningPremiumDisconnected from '../alerts/warning_premium_disconnected'
import LongConversationInfo from '../alerts/long_conversation_info'
import styles from './style.module.scss'

export interface ConversationProps {
  onIsContentReady?: (isReady: boolean) => void
}

const SUGGESTION_STATUS_SHOW_BUTTON = new Set<Mojom.SuggestionGenerationStatus>(
  [
    Mojom.SuggestionGenerationStatus.CanGenerate,
    Mojom.SuggestionGenerationStatus.IsGenerating,
  ],
)

function Conversation(props: ConversationProps) {
  const { onIsContentReady } = props
  const context = useUntrustedConversationContext()
  const state = context.api.useState().data
  const serviceState = context.api.useServiceState().data
  const premiumStatus = context.api.useGetPremiumStatus()
  const scrollElementRef = React.useRef<HTMLDivElement | null>(null)
  const contentRef = React.useRef<HTMLDivElement | null>(null)

  // Service state
  const hasAcceptedAgreement = serviceState.hasAcceptedAgreement
  const isStoragePrefEnabled = serviceState.isStoragePrefEnabled
  const isStorageNoticeDismissed = serviceState.isStorageNoticeDismissed
  const canShowPremiumPrompt = serviceState.canShowPremiumPrompt
  const isPremiumStatusFetching = premiumStatus.isPlaceholderData
  const isPremiumUser = premiumStatus.data.isPremiumUser
  const isPremiumUserDisconnected = premiumStatus.data.isPremiumUserDisconnected
  const isHistoryFeatureEnabled = true

  const showTemporaryChatInfo = state.isTemporary

  const apiHasError = state.currentError !== Mojom.APIError.None

  const shouldShowStorageNotice =
    hasAcceptedAgreement
    && isHistoryFeatureEnabled
    && isStoragePrefEnabled
    && !isStorageNoticeDismissed

  // Show premium status suggestion if the user isn't premium and chooses
  // a premium model. Secondary action is to offer to change the model.
  const shouldShowPremiumSuggestionForModel =
    hasAcceptedAgreement
    && !isPremiumStatusFetching
    && !isPremiumUser
    && state.isLeoModel
    && state.allModels.find((m) => m.key === state.currentModelKey)?.options
      .leoModelOptions?.access === Mojom.ModelAccess.PREMIUM

  // Show premium status suggestion if the user isn't premium and either the
  // service states it's a good time, or the user has attempted to perform a
  // premium-only action (e.g. regenerate a response with a premium-only model).
  // Secondary action is to dismiss the notice.
  const shouldShowPremiumSuggestionStandalone =
    hasAcceptedAgreement
    && !isPremiumStatusFetching // Avoid flash of content
    && !isPremiumUser
    && !shouldShowPremiumSuggestionForModel  // Don't show 2 premium prompts
    // service-suggestion case
    && ((!apiHasError // Don't show premium prompt and errors (rate limit error has its own premium prompt suggestion)
      && !shouldShowStorageNotice // Don't show premium prompt and storage notice
      && canShowPremiumPrompt)
      // premium-only action attempted case
      || context.showPremiumSuggestionForRegenerate)

  // Determine which error to show
  let currentErrorElement: React.ReactNode = null
  if (hasAcceptedAgreement && apiHasError) {
    switch (state.currentError) {
      case Mojom.APIError.ConnectionIssue:
        currentErrorElement = (
          <ErrorConnection
            onRetry={() => context.conversationHandler.retryAPIRequest()}
          />
        )
        break
      case Mojom.APIError.InvalidAPIKey:
        currentErrorElement = (
          <ErrorInvalidAPIKey
            onRetry={() => context.conversationHandler.retryAPIRequest()}
          />
        )
        break
      case Mojom.APIError.ServiceOverloaded:
        currentErrorElement = (
          <ErrorServiceOverloaded
            onRetry={() => context.conversationHandler.retryAPIRequest()}
          />
        )
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

  const showSuggestions: boolean =
    hasAcceptedAgreement
    && (state.suggestedQuestions.length > 0
      || SUGGESTION_STATUS_SHOW_BUTTON.has(state.suggestionStatus))

  // Total and trimmed tokens for long conversation info
  const shouldShowLongConversationInfo = state.trimmedTokens > BigInt(0)

  // Scroll tracking
  const [hasScrollableContent, setHasScrollableContent] = React.useState(false)

  React.useEffect(() => {
    const scrollElement = scrollElementRef.current
    if (!scrollElement) return

    const checkScrollable = () => {
      setHasScrollableContent(
        scrollElement.scrollHeight > scrollElement.clientHeight,
      )
    }

    checkScrollable()
    const observer = new ResizeObserver(checkScrollable)
    observer.observe(scrollElement)
    return () => observer.disconnect()
  }, [])

  const scrollToBottom = React.useCallback((animate = true) => {
    scrollElementRef.current?.scrollTo({
      top: scrollElementRef.current.scrollHeight,
      behavior: animate ? 'smooth' : 'instant',
    })
  }, [])

  // Notify parent when content is ready
  React.useEffect(() => {
    onIsContentReady?.(true)
    return () => onIsContentReady?.(false)
  }, [onIsContentReady])

  return (
    <div
      className={classnames(styles.scrollableContent, scrollerStyles.scroller)}
      ref={scrollElementRef}
    >
      <div
        className={styles.conversationContent}
        ref={contentRef}
      >
        <ModelIntro />

        {showTemporaryChatInfo && (
          <div className={styles.promptContainer}>
            <TemporaryChatInfo />
          </div>
        )}

        <ConversationEntries />

        {showSuggestions && (
          <div className={styles.suggestionsContainer}>
            <div className={styles.questionsList}>
              {state.suggestedQuestions.map((question, i) => (
                <SuggestedQuestion
                  key={question}
                  question={question}
                  index={i}
                />
              ))}
              {SUGGESTION_STATUS_SHOW_BUTTON.has(state.suggestionStatus)
                && context.associatedContent
                && context.associatedContent.length > 0 && (
                  <GenerateSuggestionsButton />
                )}
            </div>
          </div>
        )}

        {currentErrorElement && (
          <div className={styles.promptContainer}>{currentErrorElement}</div>
        )}

        {shouldShowStorageNotice && (
          <div className={styles.promptContainer}>
            <NoticeConversationStorage />
          </div>
        )}

        {shouldShowPremiumSuggestionForModel && (
          <div className={styles.promptContainer}>
            <PremiumSuggestion
              title={getLocale(S.CHAT_UI_UNLOCK_PREMIUM_TITLE)}
              secondaryActionButton={
                <Button
                  kind='plain-faint'
                  onClick={() => {
                    context.api.conversationHandler.switchToNonPremiumModel()
                  }}
                >
                  {getLocale(S.CHAT_UI_SWITCH_TO_BASIC_MODEL_BUTTON_LABEL)}
                </Button>
              }
            />
          </div>
        )}

        {shouldShowPremiumSuggestionStandalone && (
          <div className={styles.promptContainer}>
            <PremiumSuggestion
              title={getLocale(S.CHAT_UI_UNLOCK_PREMIUM_TITLE)}
              secondaryActionButton={
                <Button
                  kind='plain-faint'
                  onClick={() => {
                    // Dismiss premium prompts
                    if (context.showPremiumSuggestionForRegenerate) {
                      context.setShowPremiumSuggestionForRegenerate(false)
                    } else {
                      context.api.service.dismissPremiumPrompt()
                    }
                  }}
                >
                  {getLocale(S.CHAT_UI_DISMISS_BUTTON_LABEL)}
                </Button>
              }
            />
          </div>
        )}

        {isPremiumUserDisconnected && state.isLeoModel && (
          <div className={styles.promptContainer}>
            <WarningPremiumDisconnected />
          </div>
        )}

        {shouldShowLongConversationInfo && (
          <div className={styles.promptContainer}>
            <LongConversationInfo />
          </div>
        )}
      </div>

      <div className={styles.scrollButtonContainer}>
        <Button
          kind='outline'
          size='small'
          title={getLocale(S.CHAT_UI_SCROLL_TO_BOTTOM_BUTTON_TITLE)}
          fab
          className={classnames({
            [styles.scrollToBottomButton]: true,
            [styles.hasScrollableContent]: hasScrollableContent,
          })}
          onClick={() => scrollToBottom()}
        >
          <Icon name='arrow-down' />
        </Button>
      </div>
    </div>
  )
}

export default Conversation
