/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import AlertCenter from '@brave/leo/react/alertCenter'
import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import ConversationList from '../conversation_list'
import PrivacyMessage from '../privacy_message'
import SiteTitle from '../site_title'
import PromptAutoSuggestion from '../prompt_auto_suggestion'
import ErrorConnection from '../alerts/error_connection'
import ErrorRateLimit from '../alerts/error_rate_limit'
import InputBox from '../input_box'
import FeatureButtonMenu from '../feature_button_menu'
import ModelIntro from '../model_intro'
import PremiumSuggestion from '../premium_suggestion'
import WarningPremiumDisconnected from '../alerts/warning_premium_disconnected'
import WarningLongPage from '../alerts/warning_long_page'
import InfoLongConversation from '../alerts/info_long_conversation'
import ErrorConversationEnd from '../alerts/error_conversation_end'
import styles from './style.module.scss'

function Main() {
  const context = React.useContext(DataContext)
  const {
    siteInfo,
    userAutoGeneratePref,
    hasAcceptedAgreement,
    currentError,
    apiHasError
  } = context

  const handleEraseClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  const shouldPromptSuggestQuestions = hasAcceptedAgreement && userAutoGeneratePref === mojom.AutoGenerateQuestionsPref.Unset

  const shouldShowPremiumSuggestionForModel = hasAcceptedAgreement && !context.isPremiumUser && context.currentModel?.isPremium

  const shouldShowPremiumSuggestionStandalone =
    hasAcceptedAgreement &&
    !shouldShowPremiumSuggestionForModel && // Don't show 2 premium prompts
    !shouldPromptSuggestQuestions && // Don't show premium prompt and question prompt
    context.canShowPremiumPrompt &&
    !siteInfo &&
    !context.isPremiumUser

  const shouldDisplayEraseAction = context.conversationHistory.length >= 1

  let conversationListElement = <PrivacyMessage />
  let siteTitleElement = null
  let currentErrorElement = null

  if (hasAcceptedAgreement) {
    conversationListElement = <ConversationList />

    if (siteInfo) {
      siteTitleElement = <SiteTitle />
    }

    if (apiHasError && currentError === mojom.APIError.ConnectionIssue) {
      currentErrorElement = (
        <ErrorConnection
          onRetry={() => getPageHandlerInstance().pageHandler.retryAPIRequest()}
        />
      )
    }

    if (apiHasError && currentError === mojom.APIError.RateLimitReached) {
      currentErrorElement = (
        <ErrorRateLimit
          onRetry={() => getPageHandlerInstance().pageHandler.retryAPIRequest()}
        />
      )
    }

    if (apiHasError && currentError === mojom.APIError.ContextLimitReached) {
      currentErrorElement = (
        <ErrorConversationEnd />
      )
    }
  }

  return (
    <main className={styles.main}>
      <div className={styles.header}>
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>
            <span>Leo</span>
          </div>
          {context.isPremiumUser && <div className={styles.badgePremium}>PREMIUM</div>}
        </div>
        <div className={styles.actions}>
          {hasAcceptedAgreement && (
            <>
            {shouldDisplayEraseAction && (
              <Button
                kind='plain-faint'
                aria-label='Erase conversation history'
                title='Erase conversation history'
                onClick={handleEraseClick}
              >
                <Icon name='erase' />
              </Button>
            )}
              <FeatureButtonMenu />
            </>
          )}
        </div>
      </div>
      <div className={styles.scroller}>
        <AlertCenter position='top-left' className={styles.alertCenter} />
        {siteTitleElement && (
          <div className={styles.siteTitleBox}>{siteTitleElement}</div>
        )}
        {context.showModelIntro && <ModelIntro />}
        {conversationListElement}
        {currentErrorElement && (
          <div className={styles.promptContainer}>{currentErrorElement}</div>
        )}
        {
          shouldShowPremiumSuggestionForModel && (
            <div className={styles.promptContainer}>
              <PremiumSuggestion
                title={getLocale('unlockPremiumTitle')}
                verbose={true}
                secondaryActionButton={
                  <Button
                    kind='plain-faint'
                    onClick={() => context.dismissPremiumPrompt()}
                  >
                    {getLocale('switchToDefaultModelButtonLabel')}
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
                verbose={true}
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
        {context.shouldShowLongPageWarning &&
        <div className={styles.promptContainer}>
            <WarningLongPage />
        </div>}
        {context.shouldShowLongConversationInfo &&
        <div className={styles.promptContainer}>
            <InfoLongConversation />
        </div>}
      </div>
      <div className={styles.inputBox}>
        {shouldPromptSuggestQuestions &&
        <PromptAutoSuggestion />
        }
        <InputBox />
      </div>
    </main>
  )
}

export default Main
