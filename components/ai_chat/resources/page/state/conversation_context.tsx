// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../common/mojom'
import useIsConversationVisible from '../hooks/useIsConversationVisible'
import useSendFeedback, { defaultSendFeedbackState, SendFeedbackState } from './useSendFeedback'
import { isLeoModel } from '../model_utils'
import { tabAssociatedChatId, useActiveChat } from './active_chat_context'
import { useAIChat } from './ai_chat_context'
import getAPI from '../api'

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.8

export interface CharCountContext {
  isCharLimitExceeded: boolean
  isCharLimitApproaching: boolean
  inputTextCharCountDisplay: string
}

export type ConversationContext = SendFeedbackState & CharCountContext & {
  conversationUuid?: string
  conversationHistory: Mojom.ConversationTurn[]
  associatedContentInfo?: Mojom.SiteInfo
  allModels: Mojom.Model[]
  currentModel?: Mojom.Model
  suggestedQuestions: string[]
  isGenerating: boolean
  suggestionStatus: Mojom.SuggestionGenerationStatus
  faviconUrl?: string
  faviconCacheKey?: string
  currentError: Mojom.APIError | undefined
  apiHasError: boolean
  shouldDisableUserInput: boolean
  shouldShowLongPageWarning: boolean
  shouldShowLongConversationInfo: boolean
  shouldSendPageContents: boolean
  inputText: string
  // TODO(petemill): rename to `filteredActions`?
  actionList: Mojom.ActionGroup[]
  selectedActionType: Mojom.ActionType | undefined
  isToolsMenuOpen: boolean
  isCurrentModelLeo: boolean
  setCurrentModel: (model: Mojom.Model) => void
  switchToBasicModel: () => void
  generateSuggestedQuestions: () => void
  dismissLongConversationInfo: () => void
  updateShouldSendPageContents: (shouldSend: boolean) => void
  retryAPIRequest: () => void
  handleResetError: () => void
  setInputText: (text: string) => void
  submitInputTextToAPI: () => void
  resetSelectedActionType: () => void
  handleActionTypeClick: (actionType: Mojom.ActionType) => void
  setIsToolsMenuOpen: (isOpen: boolean) => void
  handleVoiceRecognition?: () => void
  conversationHandler?: Mojom.ConversationHandlerRemote
}

export const defaultCharCountContext: CharCountContext = {
  isCharLimitApproaching: false,
  isCharLimitExceeded: false,
  inputTextCharCountDisplay: ''
}

const defaultContext: ConversationContext = {
  conversationHistory: [],
  allModels: [],
  suggestedQuestions: [],
  isGenerating: false,
  suggestionStatus: Mojom.SuggestionGenerationStatus.None,
  apiHasError: false,
  shouldDisableUserInput: false,
  currentError: Mojom.APIError.None,
  shouldShowLongPageWarning: false,
  shouldShowLongConversationInfo: false,
  shouldSendPageContents: true,
  inputText: '',
  actionList: [],
  selectedActionType: undefined,
  isToolsMenuOpen: false,
  isCurrentModelLeo: true,
  setCurrentModel: () => { },
  switchToBasicModel: () => { },
  generateSuggestedQuestions: () => { },
  dismissLongConversationInfo: () => { },
  updateShouldSendPageContents: () => { },
  retryAPIRequest: () => { },
  handleResetError: () => { },
  setInputText: () => { },
  submitInputTextToAPI: () => { },
  resetSelectedActionType: () => { },
  handleActionTypeClick: () => { },
  setIsToolsMenuOpen: () => { },
  ...defaultSendFeedbackState,
  ...defaultCharCountContext
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

function normalizeText(text: string) {
  return text.trim().replace(/\s/g, '').toLocaleLowerCase()
}

export const getFirstValidAction = (actionList: Mojom.ActionGroup[]) =>
  actionList
    .flatMap((actionGroup) => actionGroup.entries)
    .find((entries) => entries.details)?.details?.type

export function useActionMenu(
  filter: string,
  actionList: Mojom.ActionGroup[]
) {
  return React.useMemo(() => {
    const reg = new RegExp(/^\/\w+/)

    // If we aren't filtering the actions, then just return our original list.
    if (!reg.test(filter)) return actionList

    // effectively remove the leading slash (/), and normalize before comparing it to the action labels.
    const normalizedFilter = normalizeText(filter.substring(1))

    // Filter the actionlist by our text
    return actionList
      .map((group) => ({
        ...group,
        entries: group.entries.filter(
          (entry) =>
            !!entry.details &&
            normalizeText(entry.details.label).includes(normalizedFilter)
        )
      }))
      .filter((group) => group.entries.length > 0)
  }, [actionList, filter])
}

export const ConversationReactContext =
  React.createContext<ConversationContext>(defaultContext)

export function ConversationContextProvider(props: React.PropsWithChildren) {
  const [context, setContext] =
    React.useState<ConversationContext>(defaultContext)

  const aiChatContext = useAIChat()
  const { conversationHandler, callbackRouter, selectedConversationId, updateSelectedConversationId } = useActiveChat()
  const sendFeedbackState = useSendFeedback(conversationHandler, getAPI().conversationEntriesFrameObserver)

  const [
    hasDismissedLongConversationInfo,
    setHasDismissedLongConversationInfo
  ] = React.useState<boolean>(false)

  const setPartialContext = (partialContext: Partial<ConversationContext>) => {
    setContext((value) => ({
      ...value,
      ...partialContext
    }))
  }

  const getModelContext = (
    currentModelKey: string,
    allModels: Mojom.Model[]
  ): Partial<ConversationContext> => {
    return {
      allModels,
      currentModel: allModels.find((m) => m.key === currentModelKey)
    }
  }

  // Initialization
  React.useEffect(() => {
    async function updateHistory() {
      const { conversationHistory } =
        await conversationHandler.getConversationHistory()
      setPartialContext({
        conversationHistory
      })
    }

    async function initialize() {
      const { conversationState: {
        conversationUuid,
        isRequestInProgress: isGenerating,
        allModels: models,
        currentModelKey,
        suggestedQuestions,
        suggestionStatus,
        associatedContentInfo,
        shouldSendContent,
        error
      } } = await conversationHandler.getState()
      setPartialContext({
        conversationUuid,
        isGenerating,
        ...getModelContext(currentModelKey, models),
        suggestedQuestions,
        suggestionStatus,
        associatedContentInfo,
        shouldSendPageContents: shouldSendContent,
        currentError: error
      })
    }

    // Initial data
    updateHistory()
    initialize()

    // Bind the conversation handler
    let id: number
    const listenerIds: number[] = []

    id = callbackRouter.onConversationHistoryUpdate.addListener(updateHistory)
    listenerIds.push(id)

    id = callbackRouter.onAPIRequestInProgress.addListener(
      (isGenerating: boolean) =>
        setPartialContext({
          isGenerating
        })
    )
    listenerIds.push(id)

    id = callbackRouter.onAPIResponseError.addListener((error: Mojom.APIError) =>
      setPartialContext({
        currentError: error
      })
    )
    listenerIds.push(id)

    id = callbackRouter.onModelDataChanged.addListener(
      (conversationModelKey: string, allModels: Mojom.Model[]) =>
        setPartialContext(getModelContext(conversationModelKey, allModels))
    )
    listenerIds.push(id)

    id = callbackRouter.onSuggestedQuestionsChanged.addListener(
      (questions: string[], status: Mojom.SuggestionGenerationStatus) =>
        setPartialContext({
          suggestedQuestions: questions,
          suggestionStatus: status
        })
    )
    listenerIds.push(id)

    id = callbackRouter.onAssociatedContentInfoChanged.addListener(
      (
        associatedContentInfo: Mojom.SiteInfo,
        shouldSendPageContents: boolean
      ) => {
        setPartialContext({
          associatedContentInfo,
          shouldSendPageContents
        })
      }
    )
    listenerIds.push(id)

    id = callbackRouter.onFaviconImageDataChanged.addListener(() =>
      setPartialContext({
        faviconCacheKey: new Date().getTime().toFixed(0)
      })
    )
    listenerIds.push(id)

    id = callbackRouter.onConversationDeleted.addListener(() => {
      // TODO(petemill): Show deleted UI
      console.debug('DELETED')
    })
    listenerIds.push(id)

    // Remove bindings when changing conversations
    return () => {
      for (const id of listenerIds) {
        callbackRouter.removeListener(id)
      }
    }
  }, [conversationHandler, callbackRouter])

  // Update favicon
  React.useEffect(() => {
    if (!context.conversationUuid || !aiChatContext.uiHandler) {
      return
    }
    aiChatContext.uiHandler.getFaviconImageData(context.conversationUuid)
      .then(({ faviconImageData }) => {
        if (!faviconImageData) {
          return
        }
        const blob = new Blob([new Uint8Array(faviconImageData)], { type: 'image/*' })
        setPartialContext({
          faviconUrl: URL.createObjectURL(blob)
        })
      })
  }, [context.conversationUuid, context.faviconCacheKey])

  // Update the location when the visible conversation changes
  const isVisible = useIsConversationVisible(context.conversationUuid)
  React.useEffect(() => {
    if (!isVisible) return
    if (selectedConversationId === tabAssociatedChatId) return
    if (context.conversationUuid === selectedConversationId) return
    updateSelectedConversationId(context.conversationUuid)
  }, [isVisible, updateSelectedConversationId])

  const actionList = useActionMenu(context.inputText, aiChatContext.allActions)

  const shouldShowLongConversationInfo = React.useMemo(() => {
    const chatHistoryCharTotal = context.conversationHistory.reduce(
      (charCount, curr) => charCount + curr.text.length,
      0
    )

    const options =
      context.currentModel?.options.leoModelOptions ||
      context.currentModel?.options.customModelOptions

    let totalCharLimit = 0

    if (options) {
      totalCharLimit += options.longConversationWarningCharacterLimit ?? 0
      totalCharLimit += context.shouldSendPageContents
        ? options.maxAssociatedContentLength ?? 0
        : 0
    }

    return !hasDismissedLongConversationInfo
      && chatHistoryCharTotal >= totalCharLimit
  }, [
    context.conversationHistory,
    context.currentModel,
    hasDismissedLongConversationInfo
  ])

  const apiHasError = context.currentError !== Mojom.APIError.None
  const shouldDisableUserInput = !!(
    apiHasError ||
    context.isGenerating ||
    (!aiChatContext.isPremiumUser &&
      context.currentModel?.options.leoModelOptions?.access ===
      Mojom.ModelAccess.PREMIUM)
  )
  const isCharLimitExceeded = context.inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching =
    context.inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${context.inputText.length} / ${MAX_INPUT_CHAR}`
  const isCurrentModelLeo =
    context.currentModel !== undefined && isLeoModel(context.currentModel)

  const resetSelectedActionType = () => {
    setPartialContext({
      selectedActionType: undefined
    })
  }

  const handleActionTypeClick = (actionType: Mojom.ActionType) => {
    setPartialContext({
      selectedActionType: actionType
    })
    // TODO(petemill): Explain why the settimeout?
    setTimeout(() => {
      if (context.inputText.startsWith('/')) {
        setPartialContext({
          inputText: ''
        })
      }
    })
  }

  React.useEffect(() => {
    const isOpen = context.inputText.startsWith('/') && actionList.length > 0
    setPartialContext({
      isToolsMenuOpen: isOpen
    })
  }, [context.inputText, actionList])

  const handleFilterActivation = () => {
    if (context.isToolsMenuOpen && context.inputText.startsWith('/')) {
      setPartialContext({
        selectedActionType: getFirstValidAction(actionList),
        inputText: '',
        isToolsMenuOpen: false
      })
      return true
    }

    return false
  }

  const submitInputTextToAPI = () => {
    if (!context.inputText) return
    if (isCharLimitExceeded) return
    if (shouldDisableUserInput) return
    if (handleFilterActivation()) return

    if (!aiChatContext.isStorageNoticeDismissed && aiChatContext.hasAcceptedAgreement) {
      // Submitting a conversation entry manually, after opt-in,
      // means the storage notice can be dismissed.
      aiChatContext.dismissStorageNotice()
    }

    if (context.selectedActionType) {
      conversationHandler.submitHumanConversationEntryWithAction(
        context.inputText,
        context.selectedActionType
      )
    } else {
      conversationHandler.submitHumanConversationEntry(context.inputText)
    }

    setPartialContext({
      inputText: ''
    })
    resetSelectedActionType()
  }

  // TODO(petemill): rename to switchToNonPremiumModel as there are no longer
  // a different in limitations between basic and freemium models.
  const switchToBasicModel = () => {
    // Select the first non-premium model
    const nonPremium = context.allModels.find(
      (m) => m.options.leoModelOptions?.access !== Mojom.ModelAccess.PREMIUM
    )
    if (!nonPremium) {
      console.error('Could not find a non-premium model!')
      return
    }
    conversationHandler.changeModel(nonPremium.key)
  }

  const handleResetError = async () => {
    const { turn } = await conversationHandler.clearErrorAndGetFailedMessage()
    setPartialContext({
      inputText: turn.text
    })
  }

  const handleVoiceRecognition = () => {
    if (!context.conversationUuid) {
      console.error('No conversationUuid found')
      return
    }
    aiChatContext.uiHandler?.handleVoiceRecognition(context.conversationUuid)
  }

  const store: ConversationContext = {
    ...context,
    ...sendFeedbackState,
    actionList,
    apiHasError,
    shouldDisableUserInput,
    isCharLimitApproaching,
    isCharLimitExceeded,
    inputTextCharCountDisplay,
    isCurrentModelLeo,
    shouldShowLongConversationInfo,
    dismissLongConversationInfo: () =>
      setHasDismissedLongConversationInfo(true),
    retryAPIRequest: () => conversationHandler.retryAPIRequest(),
    handleResetError,
    // Experimentally don't cache model key locally, browser should notify of model change quickly
    setCurrentModel: (model) => conversationHandler.changeModel(model.key),
    generateSuggestedQuestions: () => conversationHandler.generateQuestions(),
    resetSelectedActionType,
    updateShouldSendPageContents: (shouldSend) => conversationHandler.setShouldSendPageContents(shouldSend),
    setInputText: (inputText) => setPartialContext({ inputText }),
    handleActionTypeClick,
    submitInputTextToAPI,
    switchToBasicModel,
    setIsToolsMenuOpen: (isToolsMenuOpen) => setPartialContext({ isToolsMenuOpen }),
    handleVoiceRecognition,
    conversationHandler
  }

  return (
    <ConversationReactContext.Provider value={store}>
      {props.children}
    </ConversationReactContext.Provider>
  )
}

export function useConversation() {
  return React.useContext(ConversationReactContext)
}
