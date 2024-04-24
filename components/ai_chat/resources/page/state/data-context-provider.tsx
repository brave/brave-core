// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { loadTimeData } from '$web-common/loadTimeData'

import getPageHandlerInstance, * as mojom from '../api/page_handler'
import DataContext, { AIChatContext, ActionsList, Action } from './context'

function toBlobURL(data: number[] | null) {
  if (!data) return undefined

  const blob = new Blob([new Uint8Array(data)], { type: 'image/*' })
  return URL.createObjectURL(blob)
}

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.80

const ACTIONS_LIST: ActionsList[] = [
  {
    category: 'Quick actions',
    actions: [{ label: 'Explain', type: mojom.ActionType.EXPLAIN }]
  },
  {
    category: 'Rewrite',
    actions: [
      { label: 'Paraphrase', type: mojom.ActionType.PARAPHRASE },
      { label: 'Improve', type: mojom.ActionType.IMPROVE },
      { label: 'Change tone' },
      { label: 'Change tone / Academic', type: mojom.ActionType.ACADEMICIZE },
      {
        label: 'Change tone / Professional',
        type: mojom.ActionType.PROFESSIONALIZE
      },
      {
        label: 'Change tone / Persuasive',
        type: mojom.ActionType.PERSUASIVE_TONE
      },
      { label: 'Change tone / Casual', type: mojom.ActionType.CASUALIZE },
      { label: 'Change tone / Funny', type: mojom.ActionType.FUNNY_TONE },
      { label: 'Change length / Short', type: mojom.ActionType.SHORTEN },
      { label: 'Change length / Expand', type: mojom.ActionType.EXPAND }
    ]
  },
  {
    category: 'Create',
    actions: [
      { label: 'Tagline', type: mojom.ActionType.CREATE_TAGLINE },
      { label: 'Social media' },
      {
        label: 'Social media / Short',
        type: mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT
      },
      {
        label: 'Social media / Long',
        type: mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG
      }
    ]
  }
]

function useActionsList() {
  const [actionsList, setActionsList] = React.useState(ACTIONS_LIST)

  const filterActionsByText = (searchText: string) => {
    // effectively remove the leading slash (\) before comparing it to the action labels.
    const text = searchText.substring(1).toLocaleLowerCase()

    const filteredList = ACTIONS_LIST.map(actionList => ({
      ...actionList,
      actions: actionList.actions.filter(action => {
        // we skip non action types in the list as they're meant for display heading
        if (!('type' in action)) return
        return action.label.toLocaleLowerCase().includes(text)
      })
    })).filter(actionList => actionList.actions.length > 0)

    setActionsList(filteredList)
  }

  const resetActionsList = () => {
    setActionsList(ACTIONS_LIST)
  }

  const getFirstValidAction = (): Action => {
    const action = actionsList.flatMap(actionList => {
      return actionList.actions
    }).filter(actions => ('type' in actions))

    return action[0] as Action
  }

  return {
    actionsList,
    filterActionsByText,
    resetActionsList,
    getFirstValidAction,
  }
}

interface DataContextProviderProps {
  children: React.ReactNode
  store?: Partial<AIChatContext>
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
    hostname: undefined,
    contentUsedPercentage: 0
  })
  const [favIconUrl, setFavIconUrl] = React.useState<string>()
  const [currentError, setCurrentError] = React.useState<mojom.APIError>(mojom.APIError.None)
  const [hasAcceptedAgreement, setHasAcceptedAgreement] = React.useState(loadTimeData.getBoolean("hasAcceptedAgreement"))
  const [premiumStatus, setPremiumStatus] = React.useState<mojom.PremiumStatus | undefined>(undefined)
  const [canShowPremiumPrompt, setCanShowPremiumPrompt] = React.useState<boolean | undefined>()
  const [hasDismissedLongConversationInfo, setHasDismissedLongConversationInfo] = React.useState<boolean>(false)
  const [showAgreementModal, setShowAgreementModal] = React.useState(false)
  const [shouldSendPageContents, setShouldSendPageContents] = React.useState(true)
  const [inputText, setInputTextInternal] = React.useState('')
  const [selectedActionType, setSelectedActionType] = React.useState<mojom.ActionType | undefined>()
  const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)
  const { actionsList, filterActionsByText, resetActionsList, getFirstValidAction } = useActionsList()

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

  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${inputText.length} / ${MAX_INPUT_CHAR}`

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

  const shouldShowLongPageWarning = React.useMemo(() =>
    conversationHistory.length >= 1 &&
    siteInfo?.contentUsedPercentage < 100,
  [conversationHistory.length, siteInfo?.contentUsedPercentage])

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

  const dismissLongConversationInfo = () => {
    setHasDismissedLongConversationInfo(true)
  }

  const goPremium = () => {
    getPageHandlerInstance().pageHandler.goPremium()
  }

  const managePremium = () => {
    getPageHandlerInstance().pageHandler.managePremium()
  }

  const setInputText = (text: string) => {
    setInputTextInternal(text)

    if (text.startsWith('/')) {
      setIsToolsMenuOpen(true)
      filterActionsByText(text)
    } else {
      setIsToolsMenuOpen(false)
      resetActionsList()
    }
  }

  const handleMaybeLater = () => {
    getPageHandlerInstance().pageHandler.clearErrorAndGetFailedMessage()
      .then((res) => { setInputText(res.turn.text) })
  }

  const handleSwitchToBasicModelAndRetry = () => {
    switchToBasicModel()
    getPageHandlerInstance().pageHandler.retryAPIRequest()
  }

  const resetSelectedActionType = () => {
    setSelectedActionType(undefined)
  }

  const handleActionTypeClick = (actionType: mojom.ActionType) => {
    setSelectedActionType(actionType)

    if (inputText.startsWith('/')) {
      setInputText('')
    }
  }

  const isUserSelectingActionType = () => {
    if (
      isToolsMenuOpen &&
      inputText.startsWith('/') &&
      actionsList.length > 0
    ) {
      setSelectedActionType(getFirstValidAction().type)
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
    if (isUserSelectingActionType()) return

    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(
      inputText,
      selectedActionType ?? null
    )
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
    inputText,
    isCharLimitExceeded,
    isCharLimitApproaching,
    inputTextCharCountDisplay,
    selectedActionType,
    isToolsMenuOpen,
    actionsList,
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
    handleSwitchToBasicModelAndRetry,
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
