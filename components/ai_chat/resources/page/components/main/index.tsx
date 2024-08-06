/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import AlertCenter from '@brave/leo/react/alertCenter'
import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
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
import styles from './style.module.scss'
import ToolsButtonMenu from '../tools_button_menu'

const SCROLL_BOTTOM_THRESHOLD = 10.0

function Main() {
  const context = React.useContext(DataContext)
  const {
    siteInfo,
    hasAcceptedAgreement,
    currentError,
    apiHasError
  } = context

  const handleNewConversation = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  const shouldShowPremiumSuggestionForModel =
    hasAcceptedAgreement &&
    !context.isPremiumStatusFetching && // Avoid flash of content
    !context.isPremiumUser &&
    context.currentModel?.options.leoModelOptions?.access === mojom.ModelAccess.PREMIUM

  const shouldShowPremiumSuggestionStandalone =
    hasAcceptedAgreement &&
    !context.isPremiumStatusFetching && // Avoid flash of content
    !shouldShowPremiumSuggestionForModel && // Don't show 2 premium prompts
    !apiHasError && // Don't show premium prompt and errors (rate limit error has its own premium prompt suggestion)
    context.canShowPremiumPrompt &&
    siteInfo === null && // SiteInfo request has finished and this is a standalone conversation
    !context.isPremiumUser

  const shouldDisplayNewChatIcon = context.conversationHistory.length >= 1
  const showContextToggle = context.conversationHistory.length === 0 && siteInfo?.isContentAssociationPossible

  let currentErrorElement = null

  let scrollerElement: HTMLDivElement | null = null
  let headerElement: HTMLDivElement | null = null
  let conversationContentElement: HTMLDivElement | null = null

  const scrollPos = React.useRef({ isAtBottom: true })

  if (hasAcceptedAgreement) {
    if (apiHasError && currentError === mojom.APIError.ConnectionIssue) {
      currentErrorElement = (
        <ErrorConnection
          onRetry={() => getPageHandlerInstance().pageHandler.retryAPIRequest()}
        />
      )
    }

    if (apiHasError && currentError === mojom.APIError.RateLimitReached) {
      currentErrorElement = (
        <ErrorRateLimit />
      )
    }

    if (apiHasError && currentError === mojom.APIError.ContextLimitReached) {
      currentErrorElement = (
        <ErrorConversationEnd />
      )
    }
  }

  const handleScroll = (e: React.UIEvent<HTMLDivElement>) => {
    // Monitor scroll positions only when Assistant is generating
    if (!context.isGenerating) return
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

  let inputFocused: boolean = false
  let viewPortWithoutKeyboard = 0
  let keyboardSize = 0

  window.addEventListener('resize', () => {
    if (!context.isMobile || !inputFocused || window.visualViewport == null) {
      return
    }
    let viewPortWithKeyboard = window.visualViewport.height
    if (headerElement == null || conversationContentElement == null ||
      viewPortWithKeyboard === 0 || viewPortWithoutKeyboard === 0) {
      return
    }
    if (keyboardSize === 0 ||
        keyboardSize < viewPortWithoutKeyboard - viewPortWithKeyboard) {
      keyboardSize = viewPortWithoutKeyboard - viewPortWithKeyboard
    }
    let mountPoint: HTMLElement | null = document.getElementById('mountPoint')
    if (mountPoint !== undefined && mountPoint !== null) {
      if (mountPoint.clientHeight >=
          (headerElement.clientHeight + conversationContentElement.clientHeight)
          * 2) {
        let percent = viewPortWithKeyboard * 100 / viewPortWithoutKeyboard
        mountPoint.style.height = `${percent}%`
      } else if (keyboardSize > viewPortWithoutKeyboard - viewPortWithKeyboard) {
        mountPoint.style.height = '100%'
      }
    }
  })

  const handleOnFocusInput = () => {
    inputFocused = true
    if (window.visualViewport != null) {
      viewPortWithoutKeyboard = window.visualViewport.height
    }
  }

  const handleOnBlurInput = () => {
    let mountPoint: HTMLElement | null = document.getElementById('mountPoint')
    if (mountPoint !== undefined && mountPoint !== null
        && mountPoint.style.height !== '100%') {
      inputFocused = false
      mountPoint.style.height = '100%'
    }
  }

  return (
    <main className={styles.main}>
      {context.showAgreementModal && <PrivacyMessage />}
      <div className={styles.header}
        ref={node => {headerElement = node}}>
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>Leo AI</div>
          {context.isPremiumUser && <div className={styles.badgePremium}>PREMIUM</div>}
        </div>
        <div className={styles.actions}>
          {hasAcceptedAgreement && (
            <>
            {shouldDisplayNewChatIcon && (
              <Button
                fab
                kind='plain-faint'
                aria-label={getLocale('startConversationLabel')}
                title={getLocale('startConversationLabel')}
                onClick={handleNewConversation}
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
              onClick={() => getPageHandlerInstance().pageHandler.closePanel()}
            >
              <Icon name='close' />
            </Button>
            </>
          )}
        </div>
      </div>
      <div className={classnames({
        [styles.scroller]: true,
        [styles.flushBottom]: !hasAcceptedAgreement
      })}
        ref={node => (scrollerElement = node)}
        onScroll={handleScroll}
      >
        <AlertCenter position='top-left' className={styles.alertCenter} />
        <div className={styles.conversationContent}
          ref={node => {conversationContentElement = node}}>
          {context.hasAcceptedAgreement && <>
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
                      onClick={() => context.switchToBasicModel()}
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
                      onClick={() => context.dismissPremiumPrompt()}
                    >
                      {getLocale('dismissButtonLabel')}
                    </Button>
                  }
                />
              </div>
            )
          }
          {context.isPremiumUserDisconnected &&
          <div className={styles.promptContainer}>
            <WarningPremiumDisconnected />
          </div>
          }
          {context.shouldShowLongConversationInfo &&
          <div className={styles.promptContainer}>
              <LongConversationInfo />
          </div>}
          {!hasAcceptedAgreement && <WelcomeGuide />}
        </div>
      </div>
      <div className={styles.input}>
        {showContextToggle && (
          <div className={styles.toggleContainer}>
            <PageContextToggle />
          </div>
        )}
        <ToolsButtonMenu {...context}>
          <InputBox
            context={context}
            onFocusInput={handleOnFocusInput}
            onBlurInput={handleOnBlurInput}
          />
        </ToolsButtonMenu>
      </div>
    </main>
  )
}

export default Main
