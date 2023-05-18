/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import getPageHandlerInstance, { ConversationTurn } from '../api/page_handler'
import { loadTimeData } from '$web-common/loadTimeData'

export function useConversationHistory() {
  const [conversationHistory, setConversationHistory] = React.useState<ConversationTurn[]>([])
  const [isGenerating, setIsGenerating] = React.useState(false)
  const [suggestedQuestions, setSuggestedQuestions] = React.useState<string[]>([])

  const getConversationHistory = () => {
    getPageHandlerInstance().pageHandler.getConversationHistory().then(res => setConversationHistory(res.conversationHistory))
  }

  const getSuggestedQuestions = () => {
    getPageHandlerInstance().pageHandler.getSuggestedQuestions().then(r => setSuggestedQuestions(r.questions))
  }

  const initialiseForTargetTab = () => {
    getConversationHistory()
    getSuggestedQuestions()
  }

  React.useEffect(() => {
    initialiseForTargetTab()

    // We have a direct function for getting questions so that we only perform the expensive API operation
    // when this WebUI is active. If we only had onSuggestedQuestionsChanged, then the TabHelper
    // would always fetch questions from the remote API when the text is available
    getPageHandlerInstance().callbackRouter.onPageTextIsAvailable.addListener(getSuggestedQuestions)

    getPageHandlerInstance().callbackRouter.onConversationHistoryUpdate.addListener(getConversationHistory)
    getPageHandlerInstance().callbackRouter.onAPIRequestInProgress.addListener(setIsGenerating)
    getPageHandlerInstance().callbackRouter.onSuggestedQuestionsChanged.addListener(setSuggestedQuestions)

    // When the target tab changes, clean tab-specific data
    getPageHandlerInstance().callbackRouter.onTargetTabChanged.addListener(initialiseForTargetTab)
  }, [])

  return {
    conversationHistory,
    isGenerating,
    suggestedQuestions
  }
}

export function useInput() {
  const [value, setValue] = React.useState('')

  return {
    value,
    setValue
  }
}

export function useAgreement() {
  const [hasSeenAgreement, setHasSeenAgreement] = React.useState(loadTimeData.getBoolean("hasSeenAgreement"))

  const handleAgreeClick = () => {
    setHasSeenAgreement(true)
    getPageHandlerInstance().pageHandler.markAgreementAccepted()
  }

  return {
    hasSeenAgreement,
    handleAgreeClick
  }
}
