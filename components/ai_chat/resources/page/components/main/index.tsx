/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import ConversationList from '../conversation_list'
import PrivacyMessage from '../privacy_message'
import SiteTitle from '../site_title'
import PromptAutoSuggestion from '../prompt_auto_suggestion'
import ErrorConnection from '../error_connection'
import ErrorRateLimit from '../error_rate_limit'
import InputBox from '../input_box'
import getPageHandlerInstance, { AutoGenerateQuestionsPref, APIError } from '../../api/page_handler'
import DataContext from '../../state/context'

function Main () {
  const { siteInfo, userAutoGeneratePref, setUserAllowsAutoGenerating, generateSuggestedQuestions,
    hasSeenAgreement, currentError } = React.useContext(DataContext)

  const apiHasError = !!currentError && (currentError !== APIError.None)

  const handleQuestionSubmit = (question: string) => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(question)
  }

  const handlePageContentDisconnect = () => {
    getPageHandlerInstance().pageHandler.disconnectPageContents()
  }

  const handleOnEnableAutoGenerateQuestion = () => {
    setUserAllowsAutoGenerating(true)
    generateSuggestedQuestions()
  }

  const handleSettingsClick = () => {
    getPageHandlerInstance().pageHandler.openBraveLeoSettings()
  }

  const handleEraseClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  let conversationListElement = <PrivacyMessage />
  let siteTitleElement = null
  let promptAutoSuggestionElement = null
  let currentErrorElement = null

  if (hasSeenAgreement) {
    conversationListElement = (
      <ConversationList
        onQuestionSubmit={handleQuestionSubmit}
      />
    )

    if (siteInfo) {
      siteTitleElement = (
        <SiteTitle
          onDisconnectButtonClick={handlePageContentDisconnect}
      />
      )
    }

    if (userAutoGeneratePref === AutoGenerateQuestionsPref.Unset && userAutoGeneratePref) {
      promptAutoSuggestionElement = (
        <PromptAutoSuggestion
          onEnable={handleOnEnableAutoGenerateQuestion}
          onDismiss={() => setUserAllowsAutoGenerating(false)}
        />
      )
    }

    if (apiHasError && currentError === APIError.ConnectionIssue) {
      currentErrorElement = (
        <ErrorConnection
          onRetry={() => getPageHandlerInstance().pageHandler.retryAPIRequest()}
        />
      )
    }

    if (apiHasError && currentError === APIError.RateLimitReached) {
      currentErrorElement = (
        <ErrorRateLimit
          onRetry={() => getPageHandlerInstance().pageHandler.retryAPIRequest()}
        />
      )
    }
  }

  return (
    <main className={styles.main}>
      <div className={styles.header}>
        <div className={styles.logo}>
          <Icon name="product-brave-ai" />
          <div className={styles.logoTitle}>Brave <span>Leo</span></div>
        </div>
        <div className={styles.actions}>
          <Button kind="plain-faint" aria-label="Erase conversation history"
          title="Erase conversation history" onClick={handleEraseClick}>
              <Icon name="erase" />
          </Button>
          <Button kind="plain-faint" aria-label="Settings"
          title="Settings" onClick={handleSettingsClick}>
              <Icon name="settings" />
          </Button>
        </div>
      </div>
      <div className={styles.scroller}>
        {siteTitleElement && (
          <div className={styles.siteTitleBox}>
            {siteTitleElement}
          </div>
        )}
        {conversationListElement}
        {currentErrorElement && (
          <div className={styles.errorContainer}>
            {currentErrorElement}
          </div>
        )}
      </div>
      <div className={styles.inputBox}>
        {promptAutoSuggestionElement}
        <InputBox />
      </div>
    </main>
  )
}

export default Main
