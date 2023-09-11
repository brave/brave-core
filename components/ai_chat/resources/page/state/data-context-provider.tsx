// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { loadTimeData } from '$web-common/loadTimeData'

import getPageHandlerInstance, { ConversationTurn, AutoGenerateQuestionsPref, SiteInfo, APIError } from '../api/page_handler'
import DataContext from './context'

function toBlobURL (data: number[] | null) {
  if (!data) return undefined

  const blob = new Blob([new Uint8Array(data)], { type: 'image/*' })
  return URL.createObjectURL(blob)
}

interface DataContextProviderProps {
  children: React.ReactNode
}

function DataContextProvider (props: DataContextProviderProps) {
  const [conversationHistory, setConversationHistory] = React.useState<ConversationTurn[]>([])
  const [suggestedQuestions, setSuggestedQuestions] = React.useState<string[]>([])
  const [isGenerating, setIsGenerating] = React.useState(false)
  const [canGenerateQuestions, setCanGenerateQuestions] = React.useState(false)
  const [userAutoGeneratePref, setUserAutoGeneratePref] = React.useState<AutoGenerateQuestionsPref>()
  const [siteInfo, setSiteInfo] = React.useState<SiteInfo | null>(null)
  const [favIconUrl, setFavIconUrl] = React.useState<string>()
  const [currentError, setCurrentError] = React.useState<APIError>()
  const [hasSeenAgreement, setHasSeenAgreement] = React.useState(loadTimeData.getBoolean("hasSeenAgreement"))

  const getConversationHistory = () => {
    getPageHandlerInstance().pageHandler.getConversationHistory().then(res => setConversationHistory(res.conversationHistory))
  }

  const setUserAllowsAutoGenerating = (value: boolean) => {
    getPageHandlerInstance().pageHandler.setAutoGenerateQuestions(value)
    setUserAutoGeneratePref(value ? AutoGenerateQuestionsPref.Enabled : AutoGenerateQuestionsPref.Disabled)
  }

  const getSuggestedQuestions = () => {
    getPageHandlerInstance().pageHandler.getSuggestedQuestions().then(r => {
      setSuggestedQuestions(r.questions)
      setCanGenerateQuestions(r.canGenerate)
      setUserAutoGeneratePref(r.autoGenerate)
    })
  }

  const generateSuggestedQuestions = () => {
    getPageHandlerInstance().pageHandler.generateQuestions()
  }

  const getSiteInfo = () => {
    getPageHandlerInstance().pageHandler.getSiteInfo().then(({ siteInfo }) => {
      setSiteInfo(siteInfo)
    })
  }

  const getFaviconData = () => {
    getPageHandlerInstance().pageHandler.getFaviconImageData().then((data) => {
      setFavIconUrl(toBlobURL(data.faviconImageData))
    })
  }

  const getCurrentAPIError = () => {
    getPageHandlerInstance().pageHandler.getAPIResponseError().then((data) => {
      setCurrentError(data.error)
    })
  }

  const handleAgreeClick = () => {
    setHasSeenAgreement(true)
    getPageHandlerInstance().pageHandler.markAgreementAccepted()
  }

  const initialiseForTargetTab = () => {
    // Replace state from backend
    // TODO(petemill): Perhaps we need a simple GetState mojom function
    // and OnStateChanged event so we
    // don't need to call multiple functions or handle multiple events.
    getConversationHistory()
    getSuggestedQuestions()
    getSiteInfo()
    getFaviconData()
    getCurrentAPIError()
  }

  React.useEffect(() => {
    initialiseForTargetTab()

    getPageHandlerInstance().callbackRouter.onConversationHistoryUpdate.addListener(() => {
      getConversationHistory()
      setCanGenerateQuestions(false)
    })
    getPageHandlerInstance().callbackRouter.onAPIRequestInProgress.addListener(setIsGenerating)
    getPageHandlerInstance().callbackRouter.onSuggestedQuestionsChanged
      .addListener((questions: string[], hasGenerated: boolean, autoGenerate: AutoGenerateQuestionsPref) => {
        setSuggestedQuestions(questions)
        setCanGenerateQuestions(!hasGenerated)
        setUserAutoGeneratePref(autoGenerate)
      })

    // When the target tab changes, clean tab-specific data
    getPageHandlerInstance().callbackRouter.onTargetTabChanged.addListener(initialiseForTargetTab)

    getPageHandlerInstance().callbackRouter.onFaviconImageDataChanged.addListener((faviconImageData: number[]) => setFavIconUrl(toBlobURL(faviconImageData)))
    getPageHandlerInstance().callbackRouter.onSiteInfoChanged.addListener((siteInfo: SiteInfo) => setSiteInfo(siteInfo))
    getPageHandlerInstance().callbackRouter.onAPIResponseError.addListener((error: APIError) => setCurrentError(error))
  }, [])

  const store = {
    conversationHistory,
    isGenerating,
    suggestedQuestions,
    canGenerateQuestions,
    userAutoGeneratePref,
    siteInfo,
    favIconUrl,
    currentError,
    hasSeenAgreement,
    generateSuggestedQuestions,
    setUserAllowsAutoGenerating,
    handleAgreeClick,
  }

  return (
    <DataContext.Provider
      value={store}
    >
      {props.children}
    </DataContext.Provider>
  )
}

export default DataContextProvider
