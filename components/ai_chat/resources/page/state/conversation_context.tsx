// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import * as Mojom from '../../common/mojom'
import useSendFeedback, {
  defaultSendFeedbackState,
  SendFeedbackState,
} from './useSendFeedback'
import { isLeoModel } from '../model_utils'
import { tabAssociatedChatId, useActiveChat } from './active_chat_context'
import { useAIChat } from './ai_chat_context'
import getAPI from '../api'
import { IGNORE_EXTERNAL_LINK_WARNING_KEY } from '../../common/constants'
import {
  updateConversationHistory,
  processUploadedFilesWithLimits,
  isFullPageScreenshot,
} from '../../common/conversation_history_utils'
import useHasConversationStarted from '../hooks/useHasConversationStarted'
import { useIsDragging } from '../hooks/useIsDragging'

const MAX_INPUT_CHAR = 20000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.8

export interface CharCountContext {
  isCharLimitExceeded: boolean
  isCharLimitApproaching: boolean
  inputTextCharCountDisplay: string
}

export type UploadedImageData = Mojom.UploadedFile

export type ConversationContext = SendFeedbackState
  & CharCountContext & {
    historyInitialized: boolean
    conversationUuid?: string
    conversationHistory: Mojom.ConversationTurn[]
    associatedContentInfo: Mojom.AssociatedContent[]
    allModels: Mojom.Model[]
    currentModel?: Mojom.Model
    userDefaultModel?: Mojom.Model
    suggestedQuestions: string[]
    isGenerating: boolean
    suggestionStatus: Mojom.SuggestionGenerationStatus
    currentError: Mojom.APIError | undefined
    apiHasError: boolean
    shouldDisableUserInput: boolean
    shouldShowLongPageWarning: boolean
    shouldShowLongConversationInfo: boolean
    inputText: string
    selectedActionType: Mojom.ActionType | undefined
    selectedSkill: Mojom.Skill | undefined
    isToolsMenuOpen: boolean
    isCurrentModelLeo: boolean
    generatedUrlToBeOpened: Url | undefined
    isDragActive: boolean
    isDragOver: boolean
    unassociatedTabs: Mojom.TabData[]
    clearDragState: () => void
    setCurrentModel: (model: Mojom.Model) => void
    switchToBasicModel: () => void
    generateSuggestedQuestions: () => void
    dismissLongConversationInfo: () => void
    retryAPIRequest: () => void
    handleResetError: () => void
    handleStopGenerating: () => Promise<void>
    setInputText: (text: string) => void
    submitInputTextToAPI: () => void
    resetSelectedActionType: () => void
    resetSelectedSkill: () => void
    handleActionTypeClick: (actionType: Mojom.ActionType) => void
    handleSkillClick: (skill: Mojom.Skill) => void
    handleSkillEdit: (skill: Mojom.Skill) => void
    setIsToolsMenuOpen: (isOpen: boolean) => void
    handleVoiceRecognition?: () => void
    disassociateContent: (content: Mojom.AssociatedContent) => void
    associateDefaultContent?: () => void
    conversationHandler?: Mojom.ConversationHandlerRemote

    isTemporaryChat: boolean
    attachmentsDialog: 'tabs' | 'bookmarks' | 'history' | null
    setAttachmentsDialog: (
      show: 'tabs' | 'bookmarks' | 'history' | null,
    ) => void
    uploadFile: (useMediaCapture: boolean) => void
    getScreenshots: () => void
    removeFile: (index: number) => void
    setGeneratedUrlToBeOpened: (url?: Url) => void
    setIgnoreExternalLinkWarning: () => void
    pendingMessageFiles: Mojom.UploadedFile[]
    isUploadingFiles: boolean
    setTemporary: (temporary: boolean) => void
    attachImages: (images: Mojom.UploadedFile[]) => void
  }

export const defaultCharCountContext: CharCountContext = {
  isCharLimitApproaching: false,
  isCharLimitExceeded: false,
  inputTextCharCountDisplay: '',
}

export const defaultContext: ConversationContext = {
  historyInitialized: false,
  conversationHistory: [],
  allModels: [],
  suggestedQuestions: [],
  associatedContentInfo: [],
  isGenerating: false,
  suggestionStatus: Mojom.SuggestionGenerationStatus.None,
  apiHasError: false,
  shouldDisableUserInput: false,
  currentError: Mojom.APIError.None,
  shouldShowLongPageWarning: false,
  shouldShowLongConversationInfo: false,
  inputText: '',
  selectedActionType: undefined,
  selectedSkill: undefined,
  isToolsMenuOpen: false,
  isCurrentModelLeo: true,
  generatedUrlToBeOpened: undefined,
  isDragActive: false,
  isDragOver: false,
  unassociatedTabs: [],
  clearDragState: () => {},
  setCurrentModel: () => {},
  switchToBasicModel: () => {},
  generateSuggestedQuestions: () => {},
  dismissLongConversationInfo: () => {},
  retryAPIRequest: () => {},
  handleResetError: () => {},
  handleStopGenerating: async () => {},
  setInputText: () => {},
  submitInputTextToAPI: () => {},
  resetSelectedActionType: () => {},
  resetSelectedSkill: () => {},
  handleActionTypeClick: () => {},
  handleSkillClick: () => {},
  handleSkillEdit: () => {},
  setIsToolsMenuOpen: () => {},
  isTemporaryChat: false,
  disassociateContent: () => {},
  attachmentsDialog: null,
  setAttachmentsDialog: () => {},
  uploadFile: (useMediaCapture: boolean) => {},
  getScreenshots: () => {},
  removeFile: () => {},
  setGeneratedUrlToBeOpened: () => {},
  setIgnoreExternalLinkWarning: () => {},
  pendingMessageFiles: [],
  isUploadingFiles: false,
  setTemporary: (temporary: boolean) => {},
  attachImages: (images: Mojom.UploadedFile[]) => {},
  ...defaultSendFeedbackState,
  ...defaultCharCountContext,
}

export function useCharCountInfo(inputText: string) {
  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${inputText.length} / ${MAX_INPUT_CHAR}`

  return {
    isCharLimitExceeded,
    isCharLimitApproaching,
    inputTextCharCountDisplay,
  }
}

export const ConversationReactContext =
  React.createContext<ConversationContext>(defaultContext)

export function ConversationContextProvider(props: React.PropsWithChildren) {
  const [context, setContext] = React.useState<ConversationContext>({
    ...defaultContext,
    setAttachmentsDialog: (attachmentsDialog: 'tabs' | null) => {
      setContext((value) => ({
        ...value,
        attachmentsDialog,
      }))
    },
  })

  const aiChatContext = useAIChat()
  const {
    conversationHandler,
    callbackRouter,
    selectedConversationId,
    updateSelectedConversationId,
  } = useActiveChat()
  const sendFeedbackState = useSendFeedback(
    conversationHandler,
    getAPI().conversationEntriesFrameObserver,
  )
  const unassociatedTabs = React.useMemo(() => {
    return aiChatContext.tabs.filter(
      (t) =>
        !context.associatedContentInfo.find((c) => c.contentId === t.contentId)
          ?.conversationTurnUuid,
    )
  }, [aiChatContext.tabs, context.associatedContentInfo])

  // If there are no unassociated tabs, hide the attachments picker.
  React.useEffect(() => {
    if (unassociatedTabs.length === 0) {
      setPartialContext({
        attachmentsDialog: null,
      })
    }
  }, [unassociatedTabs])

  const [
    hasDismissedLongConversationInfo,
    setHasDismissedLongConversationInfo,
  ] = React.useState<boolean>(false)

  const setPartialContext = (partialContext: Partial<ConversationContext>) => {
    setContext((value) => ({
      ...value,
      ...partialContext,
    }))
  }

  // Drag state handlers
  const setDragActive = (isDragActive: boolean) =>
    setPartialContext({ isDragActive })
  const setDragOver = (isDragOver: boolean) => setPartialContext({ isDragOver })
  const clearDragState = () =>
    setPartialContext({ isDragActive: false, isDragOver: false })

  // Document-level and iframe drag handling
  useIsDragging({ setDragActive, setDragOver, clearDragState })

  const getModelContext = (
    currentModelKey: string,
    defaultModelKey: string,
    allModels: Mojom.Model[],
  ): Partial<ConversationContext> => {
    const availableModels =
      aiChatContext.isAIChatAgentProfileFeatureEnabled
      && aiChatContext.isAIChatAgentProfile
        ? allModels.filter((model) => model.supportsTools)
        : allModels
    return {
      allModels: availableModels,
      currentModel: allModels.find((m) => m.key === currentModelKey),
      userDefaultModel: allModels.find((m) => m.key === defaultModelKey),
    }
  }

  // Initialization
  React.useEffect(() => {
    async function updateHistory(entry?: Mojom.ConversationTurn) {
      if (entry) {
        // Use the shared utility function to update the history
        const updatedHistory = updateConversationHistory(
          context.conversationHistory,
          entry,
        )
        setPartialContext({
          conversationHistory: updatedHistory,
          historyInitialized: true,
        })
      } else {
        // When no entry is provided, fetch the full history
        const { conversationHistory } =
          await conversationHandler.getConversationHistory()
        setPartialContext({
          conversationHistory,
          historyInitialized: true,
        })
      }
    }

    async function initialize() {
      const {
        conversationState: {
          conversationUuid,
          isRequestInProgress: isGenerating,
          allModels: models,
          currentModelKey,
          defaultModelKey,
          suggestedQuestions,
          suggestionStatus,
          associatedContent,
          error,
          temporary,
        },
      } = await conversationHandler.getState()
      setPartialContext({
        conversationUuid,
        isGenerating,
        ...getModelContext(currentModelKey, defaultModelKey, models),
        suggestedQuestions,
        suggestionStatus,
        associatedContentInfo: associatedContent,
        currentError: error,
        isTemporaryChat: temporary,
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
          isGenerating,
        }),
    )
    listenerIds.push(id)

    id = callbackRouter.onAPIResponseError.addListener(
      (error: Mojom.APIError) =>
        setPartialContext({
          currentError: error,
        }),
    )
    listenerIds.push(id)

    id = callbackRouter.onModelDataChanged.addListener(
      (
        conversationModelKey: string,
        defaultModelKey: string,
        allModels: Mojom.Model[],
      ) => {
        setPartialContext(
          getModelContext(conversationModelKey, defaultModelKey, allModels),
        )
      },
    )
    listenerIds.push(id)

    id = callbackRouter.onSuggestedQuestionsChanged.addListener(
      (questions: string[], status: Mojom.SuggestionGenerationStatus) =>
        setPartialContext({
          suggestedQuestions: questions,
          suggestionStatus: status,
        }),
    )
    listenerIds.push(id)

    id = callbackRouter.onAssociatedContentInfoChanged.addListener(
      (associatedContentInfo: Mojom.AssociatedContent[]) => {
        setPartialContext({
          associatedContentInfo,
        })
      },
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

  // Update the location when the conversation has been started
  const hasConversationStarted = useHasConversationStarted(
    context.conversationUuid,
  )
  React.useEffect(() => {
    if (!hasConversationStarted) return
    if (selectedConversationId === tabAssociatedChatId) return
    if (context.conversationUuid === selectedConversationId) return
    updateSelectedConversationId(context.conversationUuid)
  }, [hasConversationStarted, updateSelectedConversationId])

  // Update page title when conversation changes
  React.useEffect(() => {
    const originalTitle = document.title
    const conversationTitle =
      aiChatContext.conversations.find(
        (c) => c.uuid === context.conversationUuid,
      )?.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)

    function setTitle(isPWA: boolean) {
      if (isPWA) {
        document.title = conversationTitle
      } else {
        document.title = `${getLocale(S.CHAT_UI_TITLE)} - ${conversationTitle}`
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
  }, [aiChatContext.conversations, context.conversationUuid])

  const shouldShowLongConversationInfo = React.useMemo(() => {
    const chatHistoryCharTotal = context.conversationHistory.reduce(
      (charCount, curr) => charCount + curr.text.length,
      0,
    )

    const options =
      context.currentModel?.options.leoModelOptions
      || context.currentModel?.options.customModelOptions

    let totalCharLimit = 0

    if (options) {
      totalCharLimit += options.longConversationWarningCharacterLimit ?? 0
      if (context.associatedContentInfo.length > 0) {
        totalCharLimit += options.maxAssociatedContentLength ?? 0
      }
    }

    return (
      !hasDismissedLongConversationInfo
      && chatHistoryCharTotal >= totalCharLimit
    )
  }, [
    context.conversationHistory,
    context.currentModel,
    hasDismissedLongConversationInfo,
  ])

  const apiHasError = context.currentError !== Mojom.APIError.None
  const shouldDisableUserInput = !!(
    apiHasError
    || context.isGenerating
    || (!aiChatContext.isPremiumUser
      && context.currentModel?.options.leoModelOptions?.access
        === Mojom.ModelAccess.PREMIUM)
  )
  const isCharLimitExceeded = context.inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching =
    context.inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${context.inputText.length} / ${MAX_INPUT_CHAR}`
  const isCurrentModelLeo =
    context.currentModel !== undefined && isLeoModel(context.currentModel)

  const resetSelectedActionType = () => {
    setPartialContext({
      selectedActionType: undefined,
    })
  }

  const resetSelectedSkill = () => {
    setPartialContext({
      selectedSkill: undefined,
    })
  }

  React.useEffect(() => {
    try {
      getAPI().metrics.onQuickActionStatusChange(!!context.selectedActionType)
    } catch (e) {}
  }, [context.selectedActionType])

  const handleActionTypeClick = (actionType: Mojom.ActionType) => {
    const update: Partial<ConversationContext> = {
      selectedActionType: actionType,
      selectedSkill: undefined,
      isToolsMenuOpen: false,
    }

    if (context.inputText.startsWith('/')) {
      update.inputText = ''
    }

    setPartialContext(update)
  }

  const handleSkillClick = (skill: Mojom.Skill) => {
    const update: Partial<ConversationContext> = {
      selectedActionType: undefined,
      selectedSkill: skill,
      isToolsMenuOpen: false,
      inputText: `/${skill.shortcut} `,
    }

    setPartialContext(update)
  }

  const handleSkillEdit = (skill: Mojom.Skill) => {
    // Close tools menu and open skill dialog in edit mode
    setPartialContext({
      isToolsMenuOpen: false,
    })

    // Open dialog with existing skill data for editing
    aiChatContext.setSkillDialog(skill)
  }

  const submitInputTextToAPI = () => {
    if (!context.inputText) return
    if (isCharLimitExceeded) return
    if (shouldDisableUserInput) return

    if (
      !aiChatContext.isStorageNoticeDismissed
      && aiChatContext.hasAcceptedAgreement
    ) {
      // Submitting a conversation entry manually, after opt-in,
      // means the storage notice can be dismissed.
      aiChatContext.dismissStorageNotice()
    }

    if (aiChatContext.isStandalone) {
      getAPI().metrics.onSendingPromptWithFullPage()
    }

    if (context.selectedSkill) {
      conversationHandler.submitHumanConversationEntryWithSkill(
        context.inputText,
        context.selectedSkill.id,
      )
    } else if (context.selectedActionType) {
      conversationHandler.submitHumanConversationEntryWithAction(
        context.inputText,
        context.selectedActionType,
      )
    } else {
      conversationHandler.submitHumanConversationEntry(
        context.inputText,
        context.pendingMessageFiles,
      )
    }

    setPartialContext({
      inputText: '',
      pendingMessageFiles: [],
    })
    resetSelectedActionType()
    resetSelectedSkill()
  }

  const disassociateContent = (content: Mojom.AssociatedContent) => {
    aiChatContext.uiHandler?.disassociateContent(
      content,
      context.conversationUuid!,
    )
  }

  const associateDefaultContent = React.useMemo(() => {
    const existingAttachedContent = context.associatedContentInfo.find(
      (c) => c.contentId === aiChatContext.defaultTabContentId,
    )
    const tab = aiChatContext.tabs.find(
      (t) => t.contentId === aiChatContext.defaultTabContentId,
    )

    return aiChatContext.defaultTabContentId && !existingAttachedContent && tab
      ? () => {
          aiChatContext.uiHandler?.associateTab(tab, context.conversationUuid!)
        }
      : undefined
  }, [
    aiChatContext.defaultTabContentId,
    aiChatContext.uiHandler,
    aiChatContext.tabs,
    context.associatedContentInfo,
    context.conversationUuid,
  ])

  // TODO(petemill): rename to switchToNonPremiumModel as there are no longer
  // a different in limitations between basic and freemium models.
  const switchToBasicModel = () => {
    // Select the first non-premium model
    const nonPremium = context.allModels.find(
      (m) => m.options.leoModelOptions?.access !== Mojom.ModelAccess.PREMIUM,
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
      inputText: turn.text,
    })
  }

  const handleStopGenerating = async () => {
    const { humanEntry } =
      await conversationHandler.stopGenerationAndMaybeGetHumanEntry()
    if (humanEntry) {
      setPartialContext({
        inputText: humanEntry.text,
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

  const processUploadedFiles = (files: Mojom.UploadedFile[]) => {
    const newFiles = processUploadedFilesWithLimits(
      files,
      context.conversationHistory,
      context.pendingMessageFiles,
    )
    if (newFiles.length > 0) {
      setPartialContext({
        isUploadingFiles: false,
        pendingMessageFiles: [...context.pendingMessageFiles, ...newFiles],
      })
    } else {
      setPartialContext({
        isUploadingFiles: false,
      })
    }
  }

  const getScreenshots = () => {
    setPartialContext({
      isUploadingFiles: true,
    })
    conversationHandler.getScreenshots().then(({ screenshots }) => {
      if (screenshots) {
        processUploadedFiles(screenshots)
      }
    })
  }

  const uploadFile = (useMediaCapture: boolean) => {
    aiChatContext.uiHandler
      ?.uploadFile(useMediaCapture)
      .then(({ uploadedFiles }) => {
        if (uploadedFiles) {
          processUploadedFiles(uploadedFiles)
        }
      })
  }

  const removeFile = (index: number) => {
    const fileToRemove = context.pendingMessageFiles[index]
    if (fileToRemove && isFullPageScreenshot(fileToRemove)) {
      // If removing a full page screenshot, remove all full page screenshots
      const updatedImages = context.pendingMessageFiles.filter(
        (file) => !isFullPageScreenshot(file),
      )
      setPartialContext({
        pendingMessageFiles: updatedImages,
      })
    } else {
      // Normal case: remove single file by index
      const updatedImages = [...context.pendingMessageFiles]
      updatedImages.splice(index, 1)
      setPartialContext({
        pendingMessageFiles: updatedImages,
      })
    }
  }

  // Listen for user uploading files to display the uploading indicator.
  React.useEffect(() => {
    async function handleSetIsUploading() {
      setPartialContext({
        isUploadingFiles: true,
      })
    }

    const listenerId =
      getAPI().uiObserver.onUploadFilesSelected.addListener(
        handleSetIsUploading,
      )

    return () => {
      getAPI().uiObserver.removeListener(listenerId)
    }
  }, [])

  const ignoreExternalLinkWarningFromLocalStorage = React.useMemo(() => {
    return JSON.parse(
      localStorage.getItem(IGNORE_EXTERNAL_LINK_WARNING_KEY) ?? 'false',
    )
  }, [])

  const ignoreExternalLinkWarning = React.useRef(
    ignoreExternalLinkWarningFromLocalStorage,
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
        localStorage.getItem(IGNORE_EXTERNAL_LINK_WARNING_KEY) ?? 'false',
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
        generatedUrlToBeOpened: url,
      })
    }

    const listenerId =
      getAPI().conversationEntriesFrameObserver.userRequestedOpenGeneratedUrl.addListener(
        handleSetOpeningExternalLinkURL,
      )

    return () => {
      getAPI().conversationEntriesFrameObserver.removeListener(listenerId)
    }
  }, [])

  // Listen for showSkillDialog requests from the child frame
  React.useEffect(() => {
    const listener = (prompt: string) => {
      aiChatContext.setSkillDialog({
        id: '',
        shortcut: '',
        prompt: prompt,
        model: '',
        createdTime: { internalValue: BigInt(0) },
        lastUsed: { internalValue: BigInt(0) },
      })
    }
    const listenerId =
      getAPI().conversationEntriesFrameObserver.showSkillDialog.addListener(
        listener,
      )

    return () => {
      getAPI().conversationEntriesFrameObserver.removeListener(listenerId)
    }
  }, [])

  const store: ConversationContext = {
    ...context,
    ...sendFeedbackState,
    apiHasError,
    shouldDisableUserInput,
    isCharLimitApproaching,
    isCharLimitExceeded,
    inputTextCharCountDisplay,
    isCurrentModelLeo,
    shouldShowLongConversationInfo,
    unassociatedTabs,
    dismissLongConversationInfo: () =>
      setHasDismissedLongConversationInfo(true),
    retryAPIRequest: () => conversationHandler.retryAPIRequest(),
    handleResetError,
    handleStopGenerating,
    // Experimentally don't cache model key locally, browser should notify of model change quickly
    setCurrentModel: (model) => conversationHandler.changeModel(model.key),
    generateSuggestedQuestions: () => conversationHandler.generateQuestions(),
    resetSelectedActionType,
    resetSelectedSkill,
    setInputText: (inputText) => setPartialContext({ inputText }),
    handleActionTypeClick,
    handleSkillClick,
    handleSkillEdit,
    submitInputTextToAPI,
    isGenerating: context.isGenerating,
    switchToBasicModel,
    setIsToolsMenuOpen: (isToolsMenuOpen) =>
      setPartialContext({ isToolsMenuOpen }),
    handleVoiceRecognition,
    uploadFile,
    getScreenshots,
    removeFile,
    setTemporary: (temporary) => {
      // Backend would check if the conversation has not yet started
      // (conversation.hasContent is false), the UI switch is only available
      // before conversation starts.
      setPartialContext({ isTemporaryChat: temporary })
      conversationHandler.setTemporary(temporary)
    },
    conversationHandler,
    setGeneratedUrlToBeOpened: (url?: Url) =>
      setPartialContext({ generatedUrlToBeOpened: url }),
    setIgnoreExternalLinkWarning,
    disassociateContent,
    associateDefaultContent,
    attachImages: (images: Mojom.UploadedFile[]) => {
      setPartialContext({
        isUploadingFiles: true,
      })
      processUploadedFiles(images)
    },
    clearDragState,
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
