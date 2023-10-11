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
import ErrorConnection from '../error_connection'
import ErrorRateLimit from '../error_rate_limit'
import InputBox from '../input_box'
import FeatureButtonMenu from '../feature_button_menu'
import styles from './style.module.scss'
import ModelIntro from '../model_intro'
import PremiumSuggestion from '../premium_suggestion'

function Main() {
  const context = React.useContext(DataContext)
  const {
    siteInfo,
    userAutoGeneratePref,
    hasSeenAgreement,
    currentError,
    apiHasError
  } = context

  const handleEraseClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  const shouldPromptSuggestQuestions = hasSeenAgreement && userAutoGeneratePref === mojom.AutoGenerateQuestionsPref.Unset

  const shouldShowPremiumSuggestionForModel = hasSeenAgreement && !context.isPremiumUser && context.currentModel?.isPremium

  const shouldShowPremiumSuggestionStandalone =
    hasSeenAgreement &&
    !shouldShowPremiumSuggestionForModel && // Don't show 2 premium prompts
    shouldPromptSuggestQuestions && // Don't show premium prompt and question prompt
    !context.hasUserDissmisedPremiumPrompt &&
    !siteInfo &&
    !context.isPremiumUser

  let conversationListElement = <PrivacyMessage />
  let siteTitleElement = null
  let currentErrorElement = null

  if (hasSeenAgreement) {
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
  }

  return (
    <main className={styles.main}>
      <div className={styles.header}>
        <div className={styles.logo}>
          <Icon name='product-brave-leo' />
          <div className={styles.logoTitle}>
            Brave <span>Leo</span>
          </div>
          {context.isPremiumUser && <div className={styles.badgePremium}>PREMIUM</div>}
        </div>
        <div className={styles.actions}>
          {hasSeenAgreement && (
            <>
              <Button
                kind='plain-faint'
                aria-label='Erase conversation history'
                title='Erase conversation history'
                onClick={handleEraseClick}
              >
                <Icon name='erase' />
              </Button>
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
        {context.hasChangedModel && <ModelIntro />}
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
