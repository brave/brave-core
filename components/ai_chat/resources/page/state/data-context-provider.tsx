// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import * as React from 'react'

import usePromise from '$web-common/usePromise'
import getPageHandlerInstance, * as mojom from '../api/page_handler'
import DataContext, { AIChatContext } from './context'

function toBlobURL(data: number[] | null) {
  if (!data) return undefined

  const blob = new Blob([new Uint8Array(data)], { type: 'image/*' })
  return URL.createObjectURL(blob)
}

function normalizeText(text: string) {
  return text.trim().replace(/\s/g, '').toLocaleLowerCase()
}

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.80

export const getFirstValidAction = (actionList: mojom.ActionGroup[]) => actionList
  .flatMap((actionGroup) => actionGroup.entries)
  .find((entries) => entries.details)?.details?.type

export function useActionMenu(filter: string, getActions: () => Promise<mojom.ActionGroup[]>) {
  const { result: actionList = [] } = usePromise(getActions, [])

  return React.useMemo(() => {
    const reg = new RegExp(/^\/\w+/)

    // If we aren't filtering the actions, then just return our original list.
    if (!reg.test(filter)) return actionList

    // effectively remove the leading slash (/), and normalize before comparing it to the action labels.
    const normalizedFilter = normalizeText(filter.substring(1))

    // Filter the actionlist by our text
    return actionList.map((group) => ({
      ...group,
      entries: group.entries.filter((entry) => !!entry.details
        && normalizeText(entry.details.label).includes(normalizedFilter))
    })).filter((group) => group.entries.length > 0);
  }, [actionList, filter])
}

export function useCharCountInfo(inputText: string) {
  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${inputText.length} / ${MAX_INPUT_CHAR}`

  return {
    isCharLimitExceeded,
    isCharLimitApproaching,
    inputTextCharCountDisplay
  }
}

interface DataContextProviderProps {
  children: React.ReactNode
  store?: Partial<AIChatContext>
}

function DataContextProvider(props: DataContextProviderProps) {
  const [currentModelKey, setCurrentModelKey] = React.useState<string>();
  const [allModels, setAllModels] = React.useState<mojom.Model[]>([])
  const [conversationHistory, setConversationHistory] = React.useState<mojom.ConversationTurn[]>([])
  const [suggestedQuestions, setSuggestedQuestions] = React.useState<string[]>([])
  const [isGenerating, setIsGenerating] =
    React.useState(props.store?.isGenerating || false)
  const [suggestionStatus, setSuggestionStatus] = React.useState<mojom.SuggestionGenerationStatus>(mojom.SuggestionGenerationStatus.None)
  const [siteInfo, setSiteInfo] = React.useState<mojom.SiteInfo>({
    title: undefined,
    isContentAssociationPossible: false,
    hostname: undefined,
    contentUsedPercentage: 0,
    isContentRefined: false
  })
  const [favIconUrl, setFavIconUrl] = React.useState<string>()
  const [currentError, setCurrentError] = React.useState<mojom.APIError>(mojom.APIError.None)
  const [hasAcceptedAgreement, setHasAcceptedAgreement] = React.useState(loadTimeData.getBoolean("hasAcceptedAgreement"))
  const [premiumStatus, setPremiumStatus] = React.useState<mojom.PremiumStatus | undefined>(undefined)
  const [canShowPremiumPrompt, setCanShowPremiumPrompt] = React.useState<boolean | undefined>()
  const [hasDismissedLongConversationInfo, setHasDismissedLongConversationInfo] = React.useState<boolean>(false)
  const [showAgreementModal, setShowAgreementModal] = React.useState(false)
  const [shouldSendPageContents, setShouldSendPageContents] = React.useState(true)
  const [inputText, setInputText] = React.useState('')
  const [selectedActionType, setSelectedActionType] = React.useState<mojom.ActionType | undefined>()
  const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)
  const actionList = useActionMenu(inputText, () => getPageHandlerInstance().pageHandler.getActionMenuList().then(({ actionList }) => actionList))

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
    const found = allModels.find(model => { return model.key === currentModelKey })

    if (!found) {
      console.error(`onModelChanged: could not find matching model for key: "${currentModelKey}"
                    in list of model keys: ${allModels.map(model => model.key).join(', ')}`)
      return
    }

    return found
  }, [allModels, currentModelKey])

  const isPremiumUser = premiumStatus !== undefined && premiumStatus !== mojom.PremiumStatus.Inactive

  const apiHasError = (currentError !== mojom.APIError.None)
  const shouldDisableUserInput = !!(apiHasError || isGenerating || (!isPremiumUser && currentModel?.options.leoModelOptions?.access === mojom.ModelAccess.PREMIUM))

  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${inputText.length} / ${MAX_INPUT_CHAR}`
  const isCurrentModelLeo = currentModel?.options.leoModelOptions !== undefined
  const isPremiumUserDisconnected = premiumStatus === mojom.PremiumStatus.ActiveDisconnected && isCurrentModelLeo

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

  // TODO(petemill): rename to switchToNonPremiumModel as there are no longer
  // a different in limitations between basic and freemium models.
  const switchToBasicModel = () => {
    // Select the first non-premium model
    const nonPremium = allModels.find(m => m.options.leoModelOptions?.access !== mojom.ModelAccess.PREMIUM)
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

  const shouldShowLongPageWarning = React.useMemo(() =>
    conversationHistory.length >= 1 &&
    siteInfo?.contentUsedPercentage < 100,
    [conversationHistory.length, siteInfo?.contentUsedPercentage])

  const shouldShowLongConversationInfo = React.useMemo(() => {
    const chatHistoryCharTotal = conversationHistory.reduce((charCount, curr) => charCount + curr.text.length, 0)

    // TODO(nullhook): make this more accurately based on the actual page content length
    let totalCharLimit =
      currentModel?.options.leoModelOptions !== undefined
        ? currentModel.options.leoModelOptions
            ?.longConversationWarningCharacterLimit
        : loadTimeData.getInteger('customModelLongConversationCharLimit')

    if (shouldSendPageContents) {
      totalCharLimit +=
        currentModel?.options.leoModelOptions !== undefined
          ? currentModel.options.leoModelOptions?.maxPageContentLength
          : loadTimeData.getInteger('customModelMaxPageContentLength')
    }

    if (
      !hasDismissedLongConversationInfo &&
      chatHistoryCharTotal >= totalCharLimit
    ) {
      return true
    }

    return false
  }, [conversationHistory, currentModel, hasDismissedLongConversationInfo, siteInfo])

  const dismissLongConversationInfo = () => {
    setHasDismissedLongConversationInfo(true)
  }

  const goPremium = () => {
    getPageHandlerInstance().pageHandler.goPremium()
  }

  const managePremium = () => {
    getPageHandlerInstance().pageHandler.managePremium()
  }

  const handleMaybeLater = () => {
    getPageHandlerInstance().pageHandler.clearErrorAndGetFailedMessage()
      .then((res) => { setInputText(res.turn.text) })
  }

  const resetSelectedActionType = () => {
    setSelectedActionType(undefined)
  }

  const handleActionTypeClick = (actionType: mojom.ActionType) => {
    setSelectedActionType(actionType)
    setTimeout(() => {
      if (inputText.startsWith('/')) {
        setInputText('')
      }
    })
  }

  React.useEffect(() => {
    const isOpen = inputText.startsWith('/') && actionList.length > 0
    setIsToolsMenuOpen(isOpen)
  }, [inputText, actionList])

  const handleFilterActivation = () => {
    if (isToolsMenuOpen && inputText.startsWith('/')) {
      setSelectedActionType(getFirstValidAction(actionList))
      setInputText('')
      setIsToolsMenuOpen(false)
      return true
    }

    return false
  }

  const submitInputTextToAPI = () => {
    if (!inputText) return
    if (isCharLimitExceeded) return
    if (shouldDisableUserInput) return
    if (handleFilterActivation()) return

    if (selectedActionType) {
      getPageHandlerInstance().pageHandler.submitHumanConversationEntryWithAction(inputText, selectedActionType)
    } else {
      getPageHandlerInstance().pageHandler.submitHumanConversationEntry(inputText)
    }

    setInputText('')
    resetSelectedActionType()
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
    resetSelectedActionType()
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

    getPageHandlerInstance().callbackRouter.onModelDataChanged.addListener((conversationModelKey: string, modelList: mojom.Model[]) => {
      setAllModels(modelList)
      setCurrentModelKey(conversationModelKey)
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
    isPremiumUserDisconnected,
    canShowPremiumPrompt,
    shouldShowLongPageWarning,
    shouldShowLongConversationInfo,
    showAgreementModal,
    shouldSendPageContents: shouldSendPageContents && siteInfo?.isContentAssociationPossible,
    isMobile,
    inputText,
    isCharLimitExceeded,
    isCharLimitApproaching,
    inputTextCharCountDisplay,
    selectedActionType,
    isToolsMenuOpen,
    actionList,
    isCurrentModelLeo,
    setCurrentModel,
    switchToBasicModel,
    goPremium,
    managePremium,
    generateSuggestedQuestions,
    handleAgreeClick,
    dismissPremiumPrompt,
    getCanShowPremiumPrompt,
    userRefreshPremiumSession,
    dismissLongConversationInfo,
    updateShouldSendPageContents,
    setInputText,
    handleMaybeLater,
    submitInputTextToAPI,
    resetSelectedActionType,
    handleActionTypeClick,
    setIsToolsMenuOpen,
    ...props.store,
  }

  return (
    <DataContext.Provider value={store}>{props.children}</DataContext.Provider>
  )
}

export default DataContextProvider
