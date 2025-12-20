// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import generateReactContext from '$web-common/api/react_api'
import { getLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { IGNORE_EXTERNAL_LINK_WARNING_KEY } from '../../common/constants'
import {
  isFullPageScreenshot,
  processUploadedFilesWithLimits,
} from '../../common/conversation_history_utils'
import * as Mojom from '../../common/mojom'
import useHasConversationStarted from '../hooks/useHasConversationStarted'
import { isLeoModel } from '../model_utils'
import { SelectedChatDetails, tabAssociatedChatId } from './active_chat_context'
import useSendFeedback, { SendFeedbackState } from './useSendFeedback'
import { useAIChat } from './ai_chat_context'
import {
  Content,
  makeEdit,
  stringifyContent,
} from '../components/input_box/editable_content'
import { useIsDragging } from '../hooks/useIsDragging'

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.8

export interface CharCountContext {
  isCharLimitExceeded: boolean
  isCharLimitApproaching: boolean
  inputTextCharCountDisplay: string
}

export type UploadedImageData = Mojom.UploadedFile

export const defaultCharCountContext: CharCountContext = {
  isCharLimitApproaching: false,
  isCharLimitExceeded: false,
  inputTextCharCountDisplay: '',
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

// Each instance of ConversationContext should be provided with an API interface
// connected to the relevant API endpoints.
type ContextProps = SelectedChatDetails

//
// Given the provided conversation API connection, provides neccessary
// react state so a conversation's UI can render.
//
export function useProvideConversationContext(props: ContextProps) {
  const [inputText, setInputText] = React.useState<Content>([])
  const [attachmentsDialog, setAttachmentsDialog] = React.useState<
    'tabs' | 'bookmarks' | 'history' | null
  >(null)
  const [selectedActionType, setSelectedActionType] =
    React.useState<Mojom.ActionType>()
  const [selectedSkill, setSelectedSkill] = React.useState<Mojom.Skill>()
  const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)
  const [pendingMessageFiles, setPendingMessageFiles] = React.useState<
    Mojom.UploadedFile[]
  >([])

  // Drag state handlers
  const [isDragActive, setDragActive] = React.useState(false)
  const [isDragOver, setDragOver] = React.useState(false)
  const clearDragState = () => setDragActive(false)
  // Document-level and iframe drag handling
  useIsDragging({ setDragActive, setDragOver, clearDragState })

  const [
    showPremiumSuggestionForRegenerate,
    setShowPremiumSuggestionForRegenerate,
  ] = React.useState(false)

  const aiChat = useAIChat()

  const { api, selectedConversationId, updateSelectedConversationId } = props
  const {
    getConversationHistoryData: conversationHistory,
    isPlaceholderData: isHistoryInitialized,
  } = api.useGetConversationHistory()
  const { getStateData: conversationState } = api.useGetState()

  const sendFeedbackState = useSendFeedback(api)
  const { tabsData } = aiChat.api.useTabs()

  const unassociatedTabs = React.useMemo(() => {
    return (
      tabsData?.filter(
        (t) =>
          !conversationState.associatedContent.find(
            (c) => c.contentId === t.contentId,
          )?.conversationTurnUuid,
      ) ?? []
    )
  }, [tabsData, conversationState.associatedContent])

  // If there are no unassociated tabs, hide the attachments picker.
  React.useEffect(() => {
    if (unassociatedTabs.length === 0 && attachmentsDialog === 'tabs') {
      setAttachmentsDialog(null)
    }
  }, [unassociatedTabs, attachmentsDialog])

  const [
    hasDismissedLongConversationInfo,
    setHasDismissedLongConversationInfo,
  ] = React.useState<boolean>(false)

  const availableModels = React.useMemo(() => {
    return aiChat.isAIChatAgentProfile && aiChat.isAIChatAgentProfile
      ? conversationState.allModels.filter((m) => m.supportsTools)
      : conversationState.allModels
  }, [
    conversationState.allModels,
    aiChat.isAIChatAgentProfile,
    aiChat.isAIChatAgentProfile,
  ])

  const currentModel = React.useMemo(() => {
    if (
      conversationState.currentModelKey
      && conversationState.allModels.length
    ) {
      return conversationState.allModels.find(
        (m) => m.key === conversationState.currentModelKey,
      )
    }
    return undefined
  }, [conversationState.currentModelKey, conversationState.allModels])

  const userDefaultModel = React.useMemo(() => {
    return conversationState.allModels.find(
      (m) => m.key === conversationState.defaultModelKey,
    )
  }, [conversationState.allModels, conversationState.defaultModelKey])

  // Update the location when the conversation has been started
  const hasConversationStarted = useHasConversationStarted(
    conversationState.conversationUuid,
  )

  React.useEffect(() => {
    if (!hasConversationStarted) return
    if (selectedConversationId === tabAssociatedChatId) return
    if (conversationState.conversationUuid === selectedConversationId) return
    updateSelectedConversationId(conversationState.conversationUuid)
  }, [hasConversationStarted, updateSelectedConversationId])

  // Update page title when conversation changes
  const conversations = aiChat.api.useGetConversations().data
  React.useEffect(() => {
    const originalTitle = document.title
    const conversationTitle =
      aiChat.conversations.find(
        (c) => c.uuid === conversationState.conversationUuid,
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
  }, [conversations, conversationState.conversationUuid])

  const shouldShowLongConversationInfo = React.useMemo<boolean>(() => {
    if (!conversationHistory || !currentModel) {
      return false
    }

    const chatHistoryCharTotal = conversationHistory.reduce(
      (charCount, curr) => charCount + curr.text.length,
      0,
    )

    const options =
      currentModel.options.leoModelOptions
      || currentModel.options.customModelOptions

    if (!options) {
      // We know this can't happen because options is a union, but someone might
      // have added an additional type to the union that we haven't accounted for.
      console.warn(
        'No options found for current model - was a new variant added to the union?',
        { currentModel },
      )
      return false
    }

    let totalCharLimit = 0

    totalCharLimit += options.longConversationWarningCharacterLimit ?? 0
    if (conversationState.associatedContent.length > 0) {
      totalCharLimit += options.maxAssociatedContentLength ?? 0
    }

    return (
      !hasDismissedLongConversationInfo
      && chatHistoryCharTotal >= totalCharLimit
    )
  }, [conversationHistory, currentModel, hasDismissedLongConversationInfo])

  const apiHasError = conversationState.error !== Mojom.APIError.None
  const shouldDisableUserInput = !!(
    apiHasError
    || conversationState.isRequestInProgress
    || (!aiChat.isPremiumUser
      && currentModel?.options.leoModelOptions?.access
        === Mojom.ModelAccess.PREMIUM)
  )
  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD
  const inputTextCharCountDisplay = `${inputText.length} / ${MAX_INPUT_CHAR}`
  const isCurrentModelLeo =
    currentModel !== undefined && isLeoModel(currentModel)

  const resetSelectedActionType = () => {
    setSelectedActionType(undefined)
  }

  React.useEffect(() => {
    try {
      aiChat.api.actions.metrics.onQuickActionStatusChange(!!selectedActionType)
    } catch (e) {}
  }, [selectedActionType])

  const handleActionTypeClick = (actionType: Mojom.ActionType) => {
    setSelectedActionType(actionType)
    setSelectedSkill(undefined)
    setIsToolsMenuOpen(false)

    const firstContent = inputText[0]
    if (typeof firstContent === 'string' && firstContent.startsWith('/')) {
      setInputText([])
    }
  }

  const handleSkillClick = (skill: Mojom.Skill) => {
    setSelectedActionType(undefined)
    setSelectedSkill(skill)
    setIsToolsMenuOpen(false)

    makeEdit(document.querySelector('[data-editor="true"]')!)
      .selectRangeToTriggerChar('/')
      .replaceSelectedRange({
        type: 'skill',
        id: skill.shortcut,
        text: `/${skill.shortcut}`,
      })
  }

  const handleSkillEdit = (skill: Mojom.Skill) => {
    // Close tools menu and open skill dialog in edit mode
    setIsToolsMenuOpen(false)

    // Open dialog with existing skill data for editing
    aiChat.setSkillDialog(skill)
  }

  const submitInputTextToAPI = () => {
    if (!inputText) return
    if (isCharLimitExceeded) return
    if (shouldDisableUserInput) return

    if (!aiChat.isStorageNoticeDismissed && aiChat.hasAcceptedAgreement) {
      // Submitting a conversation entry manually, after opt-in,
      // means the storage notice can be dismissed.
      aiChat.dismissStorageNotice()
    }

    if (aiChat.isStandalone) {
      aiChat.api.actions.metrics.onSendingPromptWithFullPage()
    }

    if (selectedSkill) {
      api.actions.submitHumanConversationEntryWithSkill(
        stringifyContent(inputText),
        selectedSkill.id,
      )
    } else if (selectedActionType) {
      api.actions.submitHumanConversationEntryWithAction(
        stringifyContent(inputText),
        selectedActionType,
      )
    } else {
      api.actions.submitHumanConversationEntry(
        stringifyContent(inputText),
        pendingMessageFiles,
      )
    }

    setInputText([])
    setPendingMessageFiles([])
    setSelectedSkill(undefined)

    resetSelectedActionType()
  }

  const disassociateContent = (content: Mojom.AssociatedContent) => {
    aiChat.api.actions.uiHandler.disassociateContent(
      content,
      conversationState.conversationUuid,
    )
  }

  const associateDefaultContent = React.useMemo(() => {
    const existingAttachedContent = conversationState.associatedContent.find(
      (c) => c.contentId === aiChat.defaultTabContentId,
    )
    const tab = tabsData?.find(
      (t) => t.contentId === aiChat.defaultTabContentId,
    )

    return aiChat.defaultTabContentId && !existingAttachedContent && tab
      ? () => {
          aiChat.api.actions.uiHandler.associateTab(
            tab,
            conversationState.conversationUuid,
          )
        }
      : undefined
  }, [
    aiChat.defaultTabContentId,
    aiChat.api.actions.uiHandler,
    tabsData,
    conversationState.associatedContent,
    conversationState.conversationUuid,
  ])

  // TODO(petemill): rename to switchToNonPremiumModel as there are no longer
  // a different in limitations between basic and freemium models.
  const switchToBasicModel = () => {
    if (showPremiumSuggestionForRegenerate) {
      setShowPremiumSuggestionForRegenerate(false)
      return
    }
    // Select the first non-premium model
    const nonPremium = conversationState.allModels.find(
      (m) => m.options.leoModelOptions?.access !== Mojom.ModelAccess.PREMIUM,
    )
    if (!nonPremium) {
      console.error('Could not find a non-premium model!')
      return
    }
    api.actions.changeModel(nonPremium.key)
  }

  const handleResetError = async () => {
    const turn = await api.clearErrorAndGetFailedMessage.mutate()
    setInputText([turn.text])
  }

  const handleStopGenerating = async () => {
    const { humanEntry } =
      await api.actions.stopGenerationAndMaybeGetHumanEntry()
    if (humanEntry) {
      setInputText([humanEntry.text])
    }
  }

  const handleVoiceRecognition = () => {
    if (!conversationState.conversationUuid) {
      console.error('No conversationUuid found')
      return
    }
    aiChat.api.actions.uiHandler.handleVoiceRecognition(
      conversationState.conversationUuid,
    )
  }

  const processUploadedFiles = async (files: Mojom.UploadedFile[]) => {
    // After mutation, any returned promise will be awaited before settling.
    // This won't re-fetch the conversation history, just get the latest
    // version if it's not invalidated.
    const conversationHistory = await api.getConversationHistory.fetch()
    const newFiles = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      pendingMessageFiles,
    )
    if (newFiles.length > 0) {
      setPendingMessageFiles([...pendingMessageFiles, ...newFiles])
    }
  }

  // Register handler for when getScreenshots is called from
  // somewhere in the UI, and also keep track of whether it's in progress.
  const screenshotsMutation = api.endpoints.getScreenshots.useMutation({
    onMutate: () => {
      // Before mutation is run, used for optimistic update
    },
    onSuccess: async (screenshots) => {
      if (screenshots) {
        return processUploadedFiles(screenshots)
      }
    },
    onSettled: async (data) => {
      // After success or error, any returned promise will be awaited before marking
      // as complete.
    },
  })

  // Register handler for when uploadFile is called
  const uploadFileMutation = aiChat.api.uploadFile.useMutation({
    onSuccess: async (uploadedFiles, [useMediaCapture]) => {
      // Reset event state, avoid us having to make a useState<bool> for this
      aiChat.api.resetOnUploadFilesSelected()
      if (uploadedFiles) {
        return processUploadedFiles(uploadedFiles)
      }
    },
  })

  // Listen for user-chosen attached files are processing
  // to display the uploading indicator only after user has chosen some files
  // (and not just cancelled the file picker).
  // Listening via useCurrentXYZ avoids having to create another state property
  const isAttachedFilesProcessing =
    aiChat.api.useCurrentOnUploadFilesSelected().hasEmitted

  // Is uploading is a combination of if files are being retrieved
  // or if chosen files are being processed.
  const isUploadingFiles =
    screenshotsMutation.isPending
    || uploadFileMutation.isPending
    || isAttachedFilesProcessing

  const removeFile = (index: number) => {
    const fileToRemove = pendingMessageFiles[index]
    if (fileToRemove && isFullPageScreenshot(fileToRemove)) {
      // If removing a full page screenshot, remove all full page screenshots
      const updatedImages = pendingMessageFiles.filter(
        (file) => !isFullPageScreenshot(file),
      )
      setPendingMessageFiles(updatedImages)
    } else {
      // Normal case: remove single file by index
      const updatedImages = [...pendingMessageFiles]
      updatedImages.splice(index, 1)
      setPendingMessageFiles(updatedImages)
    }
  }

  // Whenever input text changes, check if the selected skill has been removed
  React.useEffect(() => {
    if (!selectedSkill) return

    // If we still have the skill selected there's nothing to do.
    if (stringifyContent(inputText).startsWith(`/${selectedSkill.shortcut}`)) {
      return
    }

    // Otherwise, the user has cleared it:
    setSelectedSkill(undefined)
  }, [inputText, selectedSkill])

  // TODO(petemill): Handle this conversation's deletion:
  // Disable everything and show undismissable dialog when
  // the conversation is deleted on the backend.
  // const isDeleted = api.useCurrentOnConversationDeleted().hasEmitted

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

  // External link open requests
  const [generatedUrlToBeOpened, setGeneratedUrlToBeOpened] =
    React.useState<Url>()
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
  aiChat.api.useUserRequestedOpenGeneratedUrl((url) => {
    // If the user has ignored the warning, open the link immediately.
    if (ignoreExternalLinkWarning.current) {
      aiChat.api.actions.uiHandler.openURL(url)
      return
    }
    // Otherwise, set the URL to be opened in the modal.
    setGeneratedUrlToBeOpened(url)
  }, [])

  // Listen for showPremiumSuggestionForRegenerate requests from the child frame
  aiChat.api.useShowPremiumSuggestionForRegenerate((isVisible) => {
    setShowPremiumSuggestionForRegenerate(isVisible)
  })

  // Listen for showSkillDialog requests from the child frame
  aiChat.api.useShowSkillDialog((prompt) => {
    setSelectedSkill({
      id: '',
      shortcut: '',
      prompt: prompt,
      model: '',
      createdTime: { internalValue: BigInt(0) },
      lastUsed: { internalValue: BigInt(0) },
    })
  })

  return {
    ...sendFeedbackState,
    api: props.api,

    /**
     * @deprecated
     * Use `api.useGetState().data.conversationUuid` instead.
     */
    conversationUuid: conversationState.conversationUuid,

    /**
     * @deprecated
     * Use `api.useGetState().data.isRequestInProgress` instead.
     */
    isGenerating: conversationState.isRequestInProgress,

    allModels: availableModels,

    /**
     * @deprecated
     * Use `api.useGetState().data.associatedContent` instead.
     */
    associatedContentInfo: conversationState.associatedContent,

    /**
     * @deprecated
     * Use `api.useGetState().data.suggestedQuestions` instead.
     */
    suggestedQuestions: conversationState.suggestedQuestions,

    /**
     * @deprecated
     * Use `api.useGetState().data.suggestionStatus` instead.
     */
    suggestionStatus: conversationState.suggestionStatus,

    /**
     * @deprecated
     * Use `api.useGetState().data.error` instead.
     */
    currentError: conversationState.error,

    /**
     * @deprecated
     * This is not derived from here anymore
     * TODO: remove
     */
    shouldShowLongPageWarning: false,

    /**
     * @deprecated
     * Use `api.useGetConversationHistory().data` instead.
     */
    conversationHistory: conversationHistory,

    /**
     * @deprecated
     * Use `api.useGetConversationHistory().isPlaceholderData` instead.
     */
    historyInitialized: isHistoryInitialized,

    currentModel,
    userDefaultModel,

    attachmentsDialog,
    setAttachmentsDialog,

    inputText,
    setInputText,

    isToolsMenuOpen,
    setIsToolsMenuOpen,

    generatedUrlToBeOpened,
    setGeneratedUrlToBeOpened,
    setIgnoreExternalLinkWarning,
    disassociateContent,
    associateDefaultContent,
    attachImages: (images: Mojom.UploadedFile[]) => {
      processUploadedFiles(images)
    },

    isDragActive,
    isDragOver,
    clearDragState,

    pendingMessageFiles,

    /**
     * @deprecated
     * Use `aiChat.api.uploadFile.mutate` instead.
     */
    uploadFile: aiChat.api.uploadFile.mutate,

    /**
     * @deprecated
     * Use `api.getScreenshots.mutate` instead.
     */
    getScreenshots: api.getScreenshots.mutate,

    removeFile,
    isUploadingFiles,

    selectedSkill,
    handleSkillClick,
    handleSkillEdit,

    selectedActionType,
    apiHasError,
    shouldDisableUserInput,
    isCharLimitApproaching,
    isCharLimitExceeded,
    inputTextCharCountDisplay,
    isCurrentModelLeo,
    shouldShowLongConversationInfo,
    unassociatedTabs,
    showPremiumSuggestionForRegenerate,

    dismissLongConversationInfo: () =>
      setHasDismissedLongConversationInfo(true),
    retryAPIRequest: () => api.actions.retryAPIRequest(),
    handleResetError,
    handleStopGenerating,
    // Experimentally don't cache model key locally, browser should notify of model change quickly
    setCurrentModel: (model: Mojom.Model) => api.actions.changeModel(model.key),
    generateSuggestedQuestions: () => api.actions.generateQuestions(),
    resetSelectedActionType,
    handleActionTypeClick,
    submitInputTextToAPI,
    switchToBasicModel,
    handleVoiceRecognition,
    setTemporary: (isTemporary: boolean) => {
      // Backend would check if the conversation has not yet started
      // (conversation.hasContent is false), the UI switch is only available
      // before conversation starts.
      api.endpoints.setTemporary.mutate(isTemporary)
    },
    pauseTask: () => api.actions.pauseTask(),
    resumeTask: () => api.actions.resumeTask(),
    stopTask: () => api.actions.stopTask(),

    /**
     * @deprecated
     * Use `api.useGetState().data.toolUseTaskState` instead.
     */
    toolUseTaskState: conversationState.toolUseTaskState,
  }
}

export const { useAPI: useConversation, Provider: ConversationProvider } =
  generateReactContext(useProvideConversationContext)

export type ConversationContext = SendFeedbackState
  & CharCountContext
  & ReturnType<typeof useProvideConversationContext>

export function useConversationState() {
  // TODO: maybe the API should automatically create these when placeholder
  // data is specified?
  const conversation = useConversation()
  return conversation.api.useGetState().getStateData
}

export function useIsNewConversation() {
  const uuid = useConversationState().conversationUuid
  const aiChatContext = useAIChat()

  // A conversation is new if it isn't in the list of conversations or doesn't have content
  return !aiChatContext.conversations.find(
    (c) => c.uuid === uuid && c.hasContent,
  )
}
