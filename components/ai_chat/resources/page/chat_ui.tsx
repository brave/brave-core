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
import getPageHandlerInstance from './api/page_handler'
import SiteTitle from './components/site_title'

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

  let conversationList = <PrivacyMessage />
  let siteTitleElement = null

  if (hasSeenAgreement) {
    conversationList = (
      <>
        <ConversationList
          list={model.conversationHistory}
          isLoading={model.isGenerating}
          suggestedQuestions={model.suggestedQuestions}
          canGenerateQuestions={model.canGenerateQuestions}
          userAllowsAutoGenerating={model.userAllowsAutoGenerating}
          onSetUserAllowsAutoGenerating={model.setUserAllowsAutoGenerating}
          onGenerateSuggestedQuestions={model.generateSuggestedQuestions}
          onQuestionSubmit={handleQuestionSubmit}
        />
      </>
    )

    siteTitleElement = (
      <SiteTitle
      siteInfo={model.siteInfo}
      favIconUrl={model.favIconUrl}
    />
    )
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
      />
    </BraveCoreThemeProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
