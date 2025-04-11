// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import * as Mojom from '../../common/mojom'
import useIsConversationVisible from '../hooks/useIsConversationVisible'
import useSendFeedback, { defaultSendFeedbackState, SendFeedbackState } from './useSendFeedback'
import { isLeoModel } from '../model_utils'
import { tabAssociatedChatId, useActiveChat } from './active_chat_context'
import { useAIChat } from './ai_chat_context'
import getAPI from '../api'
import {
  IGNORE_EXTERNAL_LINK_WARNING_KEY, MAX_IMAGES //
} from '../../common/constants'

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.8

export interface CharCountContext {
  isCharLimitExceeded: boolean
  isCharLimitApproaching: boolean
  inputTextCharCountDisplay: string
}

export type UploadedImageData = Mojom.UploadedImage

export type ConversationContext = SendFeedbackState & CharCountContext & {
  historyInitialized: boolean
  conversationUuid?: string
  conversationHistory: Mojom.ConversationTurn[]
  associatedContentInfo?: Mojom.AssociatedContent
  allModels: Mojom.Model[]
  currentModel?: Mojom.Model
  suggestedQuestions: string[]
  isGenerating: boolean
  suggestionStatus: Mojom.SuggestionGenerationStatus
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
  generatedUrlToBeOpened: Url | undefined
  setCurrentModel: (model: Mojom.Model) => void
  switchToBasicModel: () => void
  generateSuggestedQuestions: () => void
  dismissLongConversationInfo: () => void
  updateShouldSendPageContents: (shouldSend: boolean) => void
  retryAPIRequest: () => void
  handleResetError: () => void
  handleStopGenerating: () => Promise<void>
  setInputText: (text: string) => void
  submitInputTextToAPI: () => void
  resetSelectedActionType: () => void
  handleActionTypeClick: (actionType: Mojom.ActionType) => void
  setIsToolsMenuOpen: (isOpen: boolean) => void
  handleVoiceRecognition?: () => void
  conversationHandler?: Mojom.ConversationHandlerRemote

  showAttachments: boolean
  setShowAttachments: (show: boolean) => void
  uploadImage: (useMediaCapture: boolean) => void
  removeImage: (index: number) => void
  setGeneratedUrlToBeOpened: (url?: Url) => void
  setIgnoreExternalLinkWarning: () => void
  pendingMessageImages: Mojom.UploadedImage[] | null
}

export const defaultCharCountContext: CharCountContext = {
  isCharLimitApproaching: false,
  isCharLimitExceeded: false,
  inputTextCharCountDisplay: ''
}

const defaultContext: ConversationContext = {
  historyInitialized: false,
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
  generatedUrlToBeOpened: undefined,
  setCurrentModel: () => { },
  switchToBasicModel: () => { },
  generateSuggestedQuestions: () => { },
  dismissLongConversationInfo: () => { },
  updateShouldSendPageContents: () => { },
  retryAPIRequest: () => { },
  handleResetError: () => { },
  handleStopGenerating: async () => { },
  setInputText: () => { },
  submitInputTextToAPI: () => { },
  resetSelectedActionType: () => { },
  handleActionTypeClick: () => { },
  setIsToolsMenuOpen: () => { },
  showAttachments: false,
  setShowAttachments: () => { },
  uploadImage: (useMediaCapture: boolean) => { },
  removeImage: () => { },
  setGeneratedUrlToBeOpened: () => { },
  setIgnoreExternalLinkWarning: () => { },
  pendingMessageImages: null,
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
    React.useState<ConversationContext>({
      ...defaultContext,
      setShowAttachments: (showAttachments: boolean) => {
        setContext((value) => ({
          ...value,
          showAttachments
        }))
      }
    })

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
        conversationHistory,
        historyInitialized: true
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
        associatedContent,
        shouldSendContent,
        error
      } } = await conversationHandler.getState()
      setPartialContext({
        conversationUuid,
        isGenerating,
        ...getModelContext(currentModelKey, models),
        suggestedQuestions,
        suggestionStatus,
        associatedContentInfo: associatedContent,
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
        associatedContentInfo: Mojom.AssociatedContent,
        shouldSendPageContents: boolean
      ) => {
        setPartialContext({
          associatedContentInfo,
          shouldSendPageContents
        })
      }
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

  // Update the location when the visible conversation changes
  const isVisible = useIsConversationVisible(context.conversationUuid)
  React.useEffect(() => {
    if (!isVisible) return
    if (selectedConversationId === tabAssociatedChatId) return
    if (context.conversationUuid === selectedConversationId) return
    updateSelectedConversationId(context.conversationUuid)
  }, [isVisible, updateSelectedConversationId])

  // Update page title when conversation changes
  React.useEffect(() => {
    const originalTitle = document.title
    const conversationTitle = aiChatContext.visibleConversations.find(c =>
      c.uuid === context.conversationUuid
    )?.title || getLocale('conversationListUntitled')

    function setTitle(isPWA: boolean) {
      if (isPWA) {
        document.title = conversationTitle
      } else {
        document.title = `${getLocale('siteTitle')} - ${conversationTitle}`
      }
    }

    const isPWAQuery = window.matchMedia('(display-mode: standalone)')
    const handleChange = (e: MediaQueryListEvent) => setTitle(e.matches)
    isPWAQuery.addEventListener('change', handleChange)

    setTitle(isPWAQuery.matches)

    return () => {
      document.title = originalTitle
      isPWAQuery.removeEventListener('change', handleChange)
    }
  }, [aiChatContext.visibleConversations, context.conversationUuid])

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

  React.useEffect(() => {
    try {
      getAPI().metrics.onQuickActionStatusChange(!!context.selectedActionType)
    } catch (e) {}
  }, [context.selectedActionType])

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

    if (aiChatContext.isStandalone) {
      getAPI().metrics.onSendingPromptWithFullPage()
    }

    if (context.selectedActionType) {
      conversationHandler.submitHumanConversationEntryWithAction(
        context.inputText,
        context.selectedActionType
      )
    } else {
      conversationHandler.submitHumanConversationEntry(
        context.inputText,
        context.pendingMessageImages)
    }

    setPartialContext({
      inputText: '',
      pendingMessageImages: null
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

  const handleStopGenerating = async () => {
    const { humanEntry } =
      await conversationHandler.stopGenerationAndMaybeGetHumanEntry()
    if (humanEntry) {
      setPartialContext({
        inputText: humanEntry.text
      })
    }
  }

  const handleVoiceRecognition = () => {
    if (!context.conversationUuid) {
      console.error('No conversationUuid found')
      return
    }
    aiChatContext.uiHandler?.handleVoiceRecognition(context.conversationUuid)
  }

  const uploadImage = (useMediaCapture: boolean) => {
    aiChatContext.uiHandler?.uploadImage(useMediaCapture)
    .then(({uploadedImages}) => {
      if (uploadedImages) {
        const totalUploadedImages = context.conversationHistory.reduce(
          (total, turn) => total + (turn.uploadedImages?.length || 0),
          0
        )
        const currentPendingImages = context.pendingMessageImages?.length || 0
        const maxNewImages = MAX_IMAGES - totalUploadedImages - currentPendingImages
        const newImages = uploadedImages.slice(0, Math.max(0, maxNewImages))

        if (newImages.length > 0) {
          setPartialContext({
            pendingMessageImages: context.pendingMessageImages
              ? [...context.pendingMessageImages, ...newImages]
              : [...newImages]
          })
        }
      }
    })
  }

  const removeImage = (index: number) => {
    if (!context.pendingMessageImages || context.pendingMessageImages.length === 0) {
      return
    }

    if (context.pendingMessageImages.length === 1) {
      setPartialContext({
        pendingMessageImages: null
      })
    } else {
      const updatedImages = [...context.pendingMessageImages]
      updatedImages.splice(index, 1)
      setPartialContext({
        pendingMessageImages: updatedImages
      })
    }
  }

  const ignoreExternalLinkWarningFromLocalStorage =
    React.useMemo(() => {
      return JSON.parse(
        localStorage.getItem(IGNORE_EXTERNAL_LINK_WARNING_KEY) ?? 'false'
      )
    }, [])

  const ignoreExternalLinkWarning = React.useRef(
    ignoreExternalLinkWarningFromLocalStorage
  )

  const setIgnoreExternalLinkWarning = () => {
    localStorage.setItem(IGNORE_EXTERNAL_LINK_WARNING_KEY, 'true')
    ignoreExternalLinkWarning.current = true
  }

  // Listen for changes to the IGNORE_EXTERNAL_LINK_WARNING_KEY key in
  // localStorage
  React.useEffect(() => {
    // Update the IGNORE_EXTERNAL_LINK_WARNING_KEY state when the key changes
    const handleStorageChange = () => {
      ignoreExternalLinkWarning.current = JSON.parse(
        localStorage.getItem(IGNORE_EXTERNAL_LINK_WARNING_KEY) ?? 'false'
      )
    }

    window.addEventListener('storage', handleStorageChange)

    return () => {
      window.removeEventListener('storage', handleStorageChange)
    }
  }, [])

  // Listen for userRequestedOpenGeneratedUrl requests from the child frame
  React.useEffect(() => {
    async function handleSetOpeningExternalLinkURL(url: Url) {
      // If the user has ignored the warning, open the link immediately.
      if (ignoreExternalLinkWarning.current) {
        getAPI().uiHandler.openURL(url)
        return
      }
      // Otherwise, set the URL to be opened in the modal.
      setPartialContext({
        generatedUrlToBeOpened: url
      })
    }

    const listenerId = getAPI()
      .conversationEntriesFrameObserver
      .userRequestedOpenGeneratedUrl
      .addListener(handleSetOpeningExternalLinkURL)

    return () => {
      getAPI()
        .conversationEntriesFrameObserver
        .removeListener(listenerId)
    }
  }, [])

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
    handleStopGenerating,
    // Experimentally don't cache model key locally, browser should notify of model change quickly
    setCurrentModel: (model) => conversationHandler.changeModel(model.key),
    generateSuggestedQuestions: () => conversationHandler.generateQuestions(),
    resetSelectedActionType,
    updateShouldSendPageContents: (shouldSend) => conversationHandler.setShouldSendPageContents(shouldSend),
    setInputText: (inputText) => setPartialContext({ inputText }),
    handleActionTypeClick,
    submitInputTextToAPI,
    isGenerating: context.isGenerating,
    switchToBasicModel,
    setIsToolsMenuOpen: (isToolsMenuOpen) => setPartialContext({ isToolsMenuOpen }),
    handleVoiceRecognition,
    uploadImage,
    removeImage,
    conversationHandler,
    setGeneratedUrlToBeOpened:
      (url?: Url) => setPartialContext({ generatedUrlToBeOpened: url }),
    setIgnoreExternalLinkWarning
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

export function useIsNewConversation() {
  const conversationContext = useConversation()
  const aiChatContext = useAIChat()

  // A conversation is new if it isn't in the list of visible conversations.
  return !aiChatContext.visibleConversations.find(c => c.uuid === conversationContext.conversationUuid)
}

export function useSupportsAttachments() {
  const aiChatContext = useAIChat()
  const isNew = useIsNewConversation()
  return aiChatContext.isStandalone && isNew
}
