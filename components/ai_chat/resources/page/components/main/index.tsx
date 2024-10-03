/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import AlertCenter from '@brave/leo/react/alertCenter'
import classnames from '$web-common/classnames'
import * as mojom from '../../api'
import { useConversation } from '../../state/conversation_context'
import { useAIChat } from '../../state/ai_chat_context'
import { isLeoModel } from '../../model_utils'
import ConversationList from '../conversation_list'
import PrivacyMessage from '../privacy_message'
import ErrorConnection from '../alerts/error_connection'
import ErrorRateLimit from '../alerts/error_rate_limit'
import InputBox from '../input_box'
import FeatureButtonMenu from '../feature_button_menu'
import ModelIntro from '../model_intro'
import PremiumSuggestion from '../premium_suggestion'
import WarningPremiumDisconnected from '../alerts/warning_premium_disconnected'
import LongConversationInfo from '../alerts/long_conversation_info'
import ErrorConversationEnd from '../alerts/error_conversation_end'
import WelcomeGuide from '../welcome_guide'
import PageContextToggle from '../page_context_toggle'
import ToolsButtonMenu from '../tools_button_menu'
import styles from './style.module.scss'

const SCROLL_BOTTOM_THRESHOLD = 10.0


function Main() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const shouldShowPremiumSuggestionForModel =
    aiChatContext.hasAcceptedAgreement &&
    !aiChatContext.isPremiumStatusFetching && // Avoid flash of content
    !aiChatContext.isPremiumUser &&
    conversationContext.currentModel?.options.leoModelOptions?.access === mojom.ModelAccess.PREMIUM

  const shouldShowPremiumSuggestionStandalone =
    aiChatContext.hasAcceptedAgreement &&
    !aiChatContext.isPremiumStatusFetching && // Avoid flash of content
    !shouldShowPremiumSuggestionForModel && // Don't show 2 premium prompts
    !conversationContext.apiHasError && // Don't show premium prompt and errors (rate limit error has its own premium prompt suggestion)
    aiChatContext.canShowPremiumPrompt &&
    conversationContext.associatedContentInfo === null && // SiteInfo request has finished and this is a standalone conversation
    !aiChatContext.isPremiumUser

  const shouldDisplayNewChatIcon = conversationContext.conversationHistory.length >= 1

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

  if (aiChatContext.hasAcceptedAgreement) {
    if (conversationContext.apiHasError && conversationContext.currentError === mojom.APIError.ConnectionIssue) {
      currentErrorElement = (
        <ErrorConnection
          onRetry={conversationContext.retryAPIRequest}
        />
      )
    }

    if (conversationContext.apiHasError && conversationContext.currentError === mojom.APIError.RateLimitReached) {
      currentErrorElement = (
        <ErrorRateLimit />
      )
    }

    if (conversationContext.apiHasError && conversationContext.currentError === mojom.APIError.ContextLimitReached) {
      currentErrorElement = (
        <ErrorConversationEnd />
      )
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
      {showAgreementModal && <PrivacyMessage />}
      <div className={styles.header}
        ref={headerElement}>
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {aiChatContext.isPremiumUser && <div className={styles.badgePremium}>PREMIUM</div>}
        </div>
        <div className={styles.actions}>
          {aiChatContext.hasAcceptedAgreement && (
            <>
            {shouldDisplayNewChatIcon && (
              <Button
                fab
                kind='plain-faint'
                aria-label={getLocale('startConversationLabel')}
                title={getLocale('startConversationLabel')}
                onClick={aiChatContext.onNewConversation}
              >
                <Icon name='edit-box' />
              </Button>
            )}
            <FeatureButtonMenu />
            <div className={styles.divider} />
            <Button
              fab
              kind='plain-faint'
              aria-label='Close'
              title='Close'
              className={styles.closeButton}
              onClick={() => aiChatContext.uiHandler?.closeUI()}
            >
              <Icon name='close' />
            </Button>
            </>
          )}
        </div>
      </div>
      <div className={classnames({
        [styles.scroller]: true,
        [styles.flushBottom]: !aiChatContext.hasAcceptedAgreement
      })}
        ref={node => (scrollerElement = node)}
        onScroll={handleScroll}
      >
        <AlertCenter position='top-left' className={styles.alertCenter} />
        <div className={styles.conversationContent}
          ref={conversationContentElement}>
          {aiChatContext.isHistoryEnabled &&
          <ul>
            <li onClick={() => aiChatContext.onSelectConversationUuid(undefined)}>This page's conversation</li>
          {aiChatContext.visibleConversations.map(conversation => (
            <li key={conversation.uuid} onClick={() => aiChatContext.onSelectConversationUuid(conversation.uuid)}>
              <div>{conversation.uuid}</div>
              <div>{conversation.title}</div>
            </li>
          ))}
          </ul>
          }
          {aiChatContext.hasAcceptedAgreement && <>
            <ModelIntro />
            <ConversationList
              onLastElementHeightChange={handleLastElementHeightChange}
            />
          </>}
          {currentErrorElement && (
            <div className={styles.promptContainer}>{currentErrorElement}</div>
          )}
          {
            shouldShowPremiumSuggestionForModel && (
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
            )
          }
          {
            shouldShowPremiumSuggestionStandalone && (
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
            )
          }
          {aiChatContext.isPremiumUserDisconnected && (!conversationContext.currentModel || isLeoModel(conversationContext.currentModel)) &&
          <div className={styles.promptContainer}>
            <WarningPremiumDisconnected />
          </div>
          }
          {conversationContext.shouldShowLongConversationInfo &&
          <div className={styles.promptContainer}>
              <LongConversationInfo />
          </div>}
          {!aiChatContext.hasAcceptedAgreement && <WelcomeGuide />}
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
            context={{...conversationContext, ...aiChatContext}}
            onFocusInputMobile={handleOnFocusInputMobile}
          />
        </ToolsButtonMenu>
      </div>
    </main>
  )
}

export default Main
