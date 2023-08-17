/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'
import { setIconBasePath } from '@brave/leo/react/icon'

import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'

import { loadTimeData } from '$web-common/loadTimeData'
import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'
import Main from './components/main'
import ConversationList from './components/conversation_list'
import InputBox from './components/input_box'
import PrivacyMessage from './components/privacy_message'
import { useConversationHistory, useInput, useAgreement } from './state/hooks'
import getPageHandlerInstance, { AutoGenerateQuestionsPref } from './api/page_handler'
import SiteTitle from './components/site_title'
import PromptAutoSuggestion from './components/prompt_auto_suggestion'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App () {
  const model = useConversationHistory()
  const { value, setValue } = useInput();
  const { hasSeenAgreement, handleAgreeClick } = useAgreement()

  const handleInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    const target = e.target as HTMLTextAreaElement
    setValue(target.value)
  }

  const handleSubmit = (e: any) => {
    if (!value) return
    e.preventDefault()
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(value)
    setValue('')
  }

  const handleQuestionSubmit = (question: string) => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(question)
  }

  const handleOnEnableAutoGenerateQuestion = () => {
    model.setUserAllowsAutoGenerating(true)
    model.generateSuggestedQuestions()
  }

  const handlePageContentDisconnect = () => {
    getPageHandlerInstance().pageHandler.disconnectPageContents()
  }

  let conversationList = <PrivacyMessage />
  let siteTitleElement = null
  let promptAutoSuggestionElement = null

  if (hasSeenAgreement) {
    conversationList = (
      <>
        <ConversationList
          list={model.conversationHistory}
          isLoading={model.isGenerating}
          suggestedQuestions={model.suggestedQuestions}
          onQuestionSubmit={handleQuestionSubmit}
        />
      </>
    )

    if (model.siteInfo) {
      siteTitleElement = (
        <SiteTitle
        siteInfo={model.siteInfo}
        favIconUrl={model.favIconUrl}
        onDisconnectButtonClick={handlePageContentDisconnect}
      />
      )
    }

    if (model.userAutoGeneratePref === AutoGenerateQuestionsPref.Unset && model.canGenerateQuestions) {
      promptAutoSuggestionElement = (
        <PromptAutoSuggestion
          onEnable={handleOnEnableAutoGenerateQuestion}
          onDismiss={() => model.setUserAllowsAutoGenerating(false)}
        />
      )
    }
  }

  const inputBox = (
    <InputBox
      value={value}
      onInputChange={handleInputChange}
      onSubmit={handleSubmit}
      onKeyDown={handleSubmit}
      hasSeenAgreement={hasSeenAgreement}
      onHandleAgreeClick={handleAgreeClick}
    />
  )

  return (
    <BraveCoreThemeProvider>
      <Main
        conversationList={conversationList}
        inputBox={inputBox}
        siteTitle={siteTitleElement}
        promptAutoSuggestion={promptAutoSuggestionElement}
        onSettingsClick={() => getPageHandlerInstance().pageHandler.openBraveLeoSettings()}
        onEraseClick={() => getPageHandlerInstance().pageHandler.clearConversationHistory()}
      />
    </BraveCoreThemeProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
