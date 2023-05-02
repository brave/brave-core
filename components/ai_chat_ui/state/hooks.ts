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
  const [hasSummarizationFailed, setHasSummarizationFailed] = React.useState(false)

  const getConversationHistory = () => {
    getPageHandlerInstance().pageHandler.getConversationHistory().then(res => setConversationHistory(res.conversationHistory))
  }

  React.useEffect(() => {
    getConversationHistory()

    getPageHandlerInstance().callbackRouter.onConversationHistoryUpdate.addListener(getConversationHistory)
    getPageHandlerInstance().callbackRouter.onAPIRequestInProgress.addListener(setIsGenerating)
    getPageHandlerInstance().callbackRouter.onContentSummarizationFailed.addListener(() => {
      setHasSummarizationFailed(true)
      setTimeout(() => setHasSummarizationFailed(false), 5000)
    })
  }, [])

  return {
    conversationHistory,
    isGenerating,
    hasSummarizationFailed
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
