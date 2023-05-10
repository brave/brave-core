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

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App () {
  const { conversationHistory, isGenerating, hasSummarizationFailed } = useConversationHistory()
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

  const handleOnSummaryClick = () => {
    getPageHandlerInstance().pageHandler.requestSummary()
  }

  let conversationList = <PrivacyMessage />

  if (hasSeenAgreement) {
    conversationList = (
      <ConversationList
        list={conversationHistory}
        isLoading={isGenerating}
      />
    )
  }

  const shouldHideSummarizationButton = !!conversationHistory.length || isGenerating || hasSummarizationFailed
  const shouldShowInput = !!conversationHistory.length

  const inputBox = (
    <InputBox
      value={value}
      showSummarizeButton={!shouldHideSummarizationButton}
      showInput={shouldShowInput}
      onInputChange={handleInputChange}
      onSubmit={handleSubmit}
      onKeyDown={handleSubmit}
      onSummaryClick={handleOnSummaryClick}
      hasSummarizationFailed={hasSummarizationFailed}
      hasSeenAgreement={hasSeenAgreement}
      onHandleAgreeClick={handleAgreeClick}
    />
  )

  return (
    <BraveCoreThemeProvider>
      <Main
        conversationList={conversationList}
        inputBox={inputBox}
      />
    </BraveCoreThemeProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
