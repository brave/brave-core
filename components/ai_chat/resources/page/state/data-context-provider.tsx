// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { loadTimeData } from '$web-common/loadTimeData'

import getPageHandlerInstance, * as mojom from '../api/page_handler'
import DataContext, { AIChatContext } from './context'

function toBlobURL(data: number[] | null) {
  if (!data) return undefined

  const blob = new Blob([new Uint8Array(data)], { type: 'image/*' })
  return URL.createObjectURL(blob)
}

interface DataContextProviderProps {
  children: React.ReactNode
}

function DataContextProvider (props: DataContextProviderProps) {
  const [currentModelKey, setCurrentModelKey] = React.useState<string>();
  const [allModels, setAllModels] = React.useState<mojom.Model[]>([])
  const [conversationHistory, setConversationHistory] = React.useState<mojom.ConversationTurn[]>([])
  const [suggestedQuestions, setSuggestedQuestions] = React.useState<string[]>([])
  const [isGenerating, setIsGenerating] = React.useState(false)
  const [suggestionStatus, setSuggestionStatus] = React.useState<mojom.SuggestionGenerationStatus>(mojom.SuggestionGenerationStatus.None)
  const [siteInfo, setSiteInfo] = React.useState<mojom.SiteInfo>({
    title: undefined,
    isContentAssociationPossible: false,
    isContentTruncated: false,
    hostname: undefined,
    truncatedContentPercentage: undefined
  })
  const [favIconUrl, setFavIconUrl] = React.useState<string>()
  const [currentError, setCurrentError] = React.useState<mojom.APIError>(mojom.APIError.None)
  const [hasAcceptedAgreement, setHasAcceptedAgreement] = React.useState(loadTimeData.getBoolean("hasAcceptedAgreement"))
  const [premiumStatus, setPremiumStatus] = React.useState<mojom.PremiumStatus | undefined>(undefined)
  const [canShowPremiumPrompt, setCanShowPremiumPrompt] = React.useState<boolean | undefined>()
  const [hasDismissedLongPageWarning, setHasDismissedLongPageWarning] = React.useState<boolean>(false)
  const [hasDismissedLongConversationInfo, setHasDismissedLongConversationInfo] = React.useState<boolean>(false)
  const [showAgreementModal, setShowAgreementModal] = React.useState(false)
  const [shouldSendPageContents, setShouldSendPageContents] = React.useState(true)

  // Provide a custom handler for setCurrentModel instead of a useEffect
  // so that we can track when the user has changed a model in
  // order to provide more information about the model.
  const setCurrentModel = (model: mojom.Model) => {
    setCurrentModelKey(model.key)
    getPageHandlerInstance().pageHandler.changeModel(model.key)
  }

  const currentModel: mojom.Model | undefined = React.useMemo(() => {
    if (!currentModelKey) {
      return
    }
    const found = allModels.find(m => m.key === currentModelKey)
    if (!found) {
      console.error(`onModelChanged: could not find matching model for key: "${currentModelKey}" in list of model keys: ${allModels.map(m => m.key).join(', ')}`)
      return
    }
    return found
  }, [allModels, currentModelKey])

  const isPremiumUser = premiumStatus !== undefined && premiumStatus !== mojom.PremiumStatus.Inactive

  const apiHasError = (currentError !== mojom.APIError.None)
  const shouldDisableUserInput = !!(apiHasError || isGenerating || (!isPremiumUser && currentModel?.access === mojom.ModelAccess.PREMIUM))

  const getConversationHistory = () => {
    getPageHandlerInstance()
      .pageHandler.getConversationHistory()
      .then((res) => setConversationHistory(res.conversationHistory))
  }

  // If a conversation entry is submitted but we haven't yet
  // accepted the policy, show the policy.
  React.useEffect(() => {
    if (conversationHistory.length && !hasAcceptedAgreement) {
      setShowAgreementModal(true)
    }
  }, [conversationHistory?.length, hasAcceptedAgreement])

  const getSuggestedQuestions = () => {
    getPageHandlerInstance()
      .pageHandler.getSuggestedQuestions()
      .then((r) => {
        setSuggestedQuestions(r.questions)
        setSuggestionStatus(r.suggestionStatus)
      })
  }

  const generateSuggestedQuestions = () => {
    getPageHandlerInstance().pageHandler.generateQuestions()
  }

  const getSiteInfo = () => {
    getPageHandlerInstance()
      .pageHandler.getSiteInfo().then(({ siteInfo }) => setSiteInfo(siteInfo))
  }

  const getFaviconData = () => {
    getPageHandlerInstance()
      .pageHandler.getFaviconImageData()
      .then((data) => {
        setFavIconUrl(toBlobURL(data.faviconImageData))
      })
  }

  const getCurrentAPIError = () => {
    getPageHandlerInstance()
      .pageHandler.getAPIResponseError()
      .then((data) => {
        setCurrentError(data.error)
      })
  }

  const handleAgreeClick = () => {
    setHasAcceptedAgreement(true)
    setShowAgreementModal(false)
    getPageHandlerInstance().pageHandler.markAgreementAccepted()
  }

  const getCanShowPremiumPrompt = () => {
    getPageHandlerInstance().pageHandler.getCanShowPremiumPrompt()
      .then(resp => setCanShowPremiumPrompt(resp.canShow))
  }

  const dismissPremiumPrompt = () => {
    getPageHandlerInstance().pageHandler.dismissPremiumPrompt()
    setCanShowPremiumPrompt(false)
  }

  const switchToBasicModel = () => {
    // Select the first non-premium model
    const nonPremium = allModels.find(m => m.access === mojom.ModelAccess.BASIC)
    if (!nonPremium) {
      console.error('Could not find a non-premium model!')
      return
    }
    setCurrentModel(nonPremium)
  }

  const updateCurrentPremiumStatus = async () => {
    console.debug('Getting premium status...')
    const { status } = await getPageHandlerInstance()
      .pageHandler.getPremiumStatus()
    console.debug('got premium status: ', status)
    setPremiumStatus(status)
  }

  const userRefreshPremiumSession = () => {
    getPageHandlerInstance().pageHandler.refreshPremiumSession()
  }

  const updateShouldSendPageContents = (shouldSend: boolean) => {
    setShouldSendPageContents(shouldSend)
    getPageHandlerInstance().pageHandler.setShouldSendPageContents(shouldSend)
  }

  const getShouldSendPageContents = () => {
    getPageHandlerInstance().pageHandler.getShouldSendPageContents().then(({ shouldSend }) => setShouldSendPageContents(shouldSend))
  }

  const shouldShowLongPageWarning = React.useMemo(() => {
    if (
      !hasDismissedLongPageWarning &&
      conversationHistory.length >= 1 &&
      siteInfo?.isContentTruncated
    ) {
      return true
    }

    return false
  }, [conversationHistory, hasDismissedLongPageWarning, siteInfo?.isContentTruncated])

  const shouldShowLongConversationInfo = React.useMemo(() => {
    if (!currentModel) return false

    const chatHistoryCharTotal = conversationHistory.reduce((charCount, curr) => charCount + curr.text.length, 0)

    // TODO(nullhook): make this more accurately based on the actual page content length
    let totalCharLimit = currentModel?.longConversationWarningCharacterLimit
    if (shouldSendPageContents) totalCharLimit += currentModel?.maxPageContentLength

    if (
      !hasDismissedLongConversationInfo &&
      chatHistoryCharTotal >= totalCharLimit
    ) {
      return true
    }

    return false
  }, [conversationHistory, currentModel, hasDismissedLongConversationInfo, siteInfo])

  const dismissLongPageWarning = () => {
    setHasDismissedLongPageWarning(true)
  }

  const dismissLongConversationInfo = () => {
    setHasDismissedLongConversationInfo(true)
  }

  const goPremium = () => {
    getPageHandlerInstance().pageHandler.goPremium()
  }

  const managePremium = () => {
    getPageHandlerInstance().pageHandler.managePremium()
  }

  const initialiseForTargetTab = async () => {
    // Replace state from backend
    // TODO(petemill): Perhaps we need a simple GetState mojom function
    // and OnStateChanged event so we
    // don't need to call multiple functions or handle multiple events.
    updateCurrentPremiumStatus()
    getConversationHistory()
    getSuggestedQuestions()
    getSiteInfo()
    getFaviconData()
    getCurrentAPIError()
    getCanShowPremiumPrompt()
    getShouldSendPageContents()
  }

  const isMobile = React.useMemo(() => loadTimeData.getBoolean('isMobile'), [])

  React.useEffect(() => {
    initialiseForTargetTab()

    // This never changes
    getPageHandlerInstance().pageHandler.getModels().then(data => {
      setAllModels(data.models)
      setCurrentModelKey(data.currentModelKey);
    })

    // Setup data event handlers
    getPageHandlerInstance().callbackRouter.onConversationHistoryUpdate.addListener(() => {
      getConversationHistory()
      getShouldSendPageContents()
    })
    getPageHandlerInstance().callbackRouter.onAPIRequestInProgress.addListener(setIsGenerating)
    getPageHandlerInstance().callbackRouter.onSuggestedQuestionsChanged
      .addListener((questions: string[], suggestionStatus: mojom.SuggestionGenerationStatus) => {
        setSuggestedQuestions(questions)
        setSuggestionStatus(suggestionStatus)
      }
    )
    getPageHandlerInstance().callbackRouter.onFaviconImageDataChanged.addListener((faviconImageData: number[]) => setFavIconUrl(toBlobURL(faviconImageData)))
    getPageHandlerInstance().callbackRouter.onSiteInfoChanged.addListener(
      setSiteInfo
    )
    getPageHandlerInstance().callbackRouter.onAPIResponseError.addListener((error: mojom.APIError) => setCurrentError(error))

    getPageHandlerInstance().callbackRouter.onModelChanged.addListener((modelKey: string) => {
      setCurrentModelKey(modelKey)
    })

    // Since there is no server-side event for premium status changing,
    // we should check often. And since purchase or login is performed in
    // a separate WebContents, we can check when focus is returned here.
    window.addEventListener('focus', () => {
      updateCurrentPremiumStatus()
    })

    document.addEventListener('visibilitychange', (e) => {
      if (document.visibilityState === 'visible') {
        updateCurrentPremiumStatus()
      }
    })
  }, [])

  const store: AIChatContext = {
    allModels,
    currentModel,
    conversationHistory,
    isGenerating,
    suggestedQuestions,
    suggestionStatus,
    siteInfo,
    favIconUrl,
    currentError,
    hasAcceptedAgreement,
    apiHasError,
    shouldDisableUserInput,
    isPremiumStatusFetching: premiumStatus === undefined,
    isPremiumUser,
    isPremiumUserDisconnected: premiumStatus === mojom.PremiumStatus.ActiveDisconnected,
    canShowPremiumPrompt,
    shouldShowLongPageWarning,
    shouldShowLongConversationInfo,
    showAgreementModal,
    shouldSendPageContents: shouldSendPageContents && siteInfo?.isContentAssociationPossible,
    isMobile,
    setCurrentModel,
    switchToBasicModel,
    goPremium,
    managePremium,
    generateSuggestedQuestions,
    handleAgreeClick,
    dismissPremiumPrompt,
    getCanShowPremiumPrompt,
    userRefreshPremiumSession,
    dismissLongPageWarning,
    dismissLongConversationInfo,
    updateShouldSendPageContents,
  }

  return (
    <DataContext.Provider value={store}>{props.children}</DataContext.Provider>
  )
}

export default DataContextProvider
