// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { loadTimeData } from '$web-common/loadTimeData'

import getPageHandlerInstance, * as mojom from '../api/page_handler'
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
  const [currentModel, setCurrentModelRaw] = React.useState<mojom.Model>();
  const [allModels, setAllModels] = React.useState<mojom.Model[]>([])
  const [hasChangedModel, setHasChangedModel] = React.useState(false)
  const [conversationHistory, setConversationHistory] = React.useState<mojom.ConversationTurn[]>([])
  const [suggestedQuestions, setSuggestedQuestions] = React.useState<string[]>([])
  const [isGenerating, setIsGenerating] = React.useState(false)
  const [canGenerateQuestions, setCanGenerateQuestions] = React.useState(false)
  const [userAutoGeneratePref, setUserAutoGeneratePref] = React.useState<mojom.AutoGenerateQuestionsPref>()
  const [siteInfo, setSiteInfo] = React.useState<mojom.SiteInfo | null>(null)
  const [favIconUrl, setFavIconUrl] = React.useState<string>()
  const [currentError, setCurrentError] = React.useState<mojom.APIError>(mojom.APIError.None)
  const [hasSeenAgreement, setHasSeenAgreement] = React.useState(loadTimeData.getBoolean("hasSeenAgreement"))

  // Provide a custom handler for setCurrentModel instead of a useEffect
  // so that we can track when the user has changed a model in
  // order to provide more information about the model.
  const setCurrentModel = (model: mojom.Model) => {
    setHasChangedModel(true)
    setCurrentModelRaw(model)
    getPageHandlerInstance().pageHandler.changeModel(model.key)
  }

  const apiHasError = (currentError !== mojom.APIError.None)
  const shouldDisableUserInput = apiHasError || isGenerating

  const getConversationHistory = () => {
    getPageHandlerInstance().pageHandler.getConversationHistory().then(res => setConversationHistory(res.conversationHistory))
  }

  const setUserAllowsAutoGenerating = (value: boolean) => {
    getPageHandlerInstance().pageHandler.setAutoGenerateQuestions(value)
    setUserAutoGeneratePref(value ? mojom.AutoGenerateQuestionsPref.Enabled : mojom.AutoGenerateQuestionsPref.Disabled)
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

    // This never changes
    getPageHandlerInstance().pageHandler.getModels().then(data => {
      setAllModels(data.models)
      setCurrentModelRaw(data.currentModel);
    })

    getPageHandlerInstance().callbackRouter.onConversationHistoryUpdate.addListener(() => {
      getConversationHistory()
      setCanGenerateQuestions(false)
    })
    getPageHandlerInstance().callbackRouter.onAPIRequestInProgress.addListener(setIsGenerating)
    getPageHandlerInstance().callbackRouter.onSuggestedQuestionsChanged
      .addListener((questions: string[], hasGenerated: boolean, autoGenerate: mojom.AutoGenerateQuestionsPref) => {
        setSuggestedQuestions(questions)
        setCanGenerateQuestions(!hasGenerated)
        setUserAutoGeneratePref(autoGenerate)
      })

    getPageHandlerInstance().callbackRouter.onFaviconImageDataChanged.addListener((faviconImageData: number[]) => setFavIconUrl(toBlobURL(faviconImageData)))
    getPageHandlerInstance().callbackRouter.onSiteInfoChanged.addListener((siteInfo: mojom.SiteInfo) => setSiteInfo(siteInfo))
    getPageHandlerInstance().callbackRouter.onAPIResponseError.addListener((error: mojom.APIError) => setCurrentError(error))
  }, [])

  const store = {
    allModels,
    currentModel,
    hasChangedModel,
    conversationHistory,
    isGenerating,
    suggestedQuestions,
    canGenerateQuestions,
    userAutoGeneratePref,
    siteInfo,
    favIconUrl,
    currentError,
    hasSeenAgreement,
    apiHasError,
    shouldDisableUserInput,
    setCurrentModel,
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
