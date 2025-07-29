/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as Mojom from '../../../../components/ai_chat/resources/common/mojom'
import {
  AIChatReactContext,
  AIChatContext
} from '../../../../components/ai_chat/resources/page/state/ai_chat_context'
import StorybookConversationEntries from '../../../../components/ai_chat/resources/page/stories/story_utils/ConversationEntries'
import ACTIONS_LIST from '../../../../components/ai_chat/resources/page/stories/story_utils/actions'
import {
  ActiveChatContext,
  SelectedChatDetails,
} from '../../../../components/ai_chat/resources/page/state/active_chat_context'
import {
  ConversationContext,
  ConversationReactContext,
} from '../../../../components/ai_chat/resources/page/state/conversation_context'
import {
  Content,
  stringifyContent
} from '../../../../components/ai_chat/resources/page/components/input_box/editable_content'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

const CONVERSATIONS: Mojom.Conversation[] = [
  {
    title: 'Star Trek Poem',
    uuid: '1',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000000') },
    associatedContent: [],
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
  },
  {
    title:
      'Sorting C++ vectors is hard especially when you have to have a very long title for your conversation to test text clipping or wrapping',
    uuid: '2',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000001') },
    associatedContent: [],
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
  },
  {
    title: '',
    uuid: '3',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000002') },
    associatedContent: [],
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
  },
]

const MODELS: Mojom.Model[] = [
  {
    key: '1',
    displayName: 'Model One',
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: true,
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: '2',
    displayName: 'Model Two',
    visionSupport: true,
    supportsTools: true,
    isSuggestedModel: true,
    options: {
      leoModelOptions: {
        name: 'model-two-premium',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: '3',
    displayName: 'Model Three',
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: false,
    options: {
      leoModelOptions: {
        name: 'model-three-freemium',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: '4',
    displayName: 'Microsoft Phi-3',
    visionSupport: false,
    supportsTools: true,
    isSuggestedModel: false,
    options: {
      leoModelOptions: undefined,
      customModelOptions: {
        modelRequestName: 'phi3',
        contextSize: 131072,
        // The maxAssociatedContentLength (131072 tokens * 4 chars per token)
        // and longConversationWarningCharacterLimit (60% of
        // maxAssociatedContentLength) are both here only to satisfy the
        // type checker.
        maxAssociatedContentLength: 131072 * 4,
        longConversationWarningCharacterLimit: 131072 * 4 * 0.6,
        modelSystemPrompt:
          'You are a helpful AI assistant who wishes not to be '
          + 'associated with a character named Clippy.',
        endpoint: { url: 'https://example.com' },
        apiKey: '123456',
      },
    },
  },
]

type CustomArgs = {
  initialized: boolean
  currentErrorState: keyof typeof Mojom.APIError
  model: string
  inputText: Content
  hasConversation: boolean
  editingConversationId: string | null
  deletingConversationId: string | null
  conversationListCount: number
  hasSuggestedQuestions: boolean
  hasAssociatedContent: boolean
  isFeedbackFormVisible: boolean
  isStorageNoticeDismissed: boolean
  canShowPremiumPrompt: boolean
  hasAcceptedAgreement: boolean
  isStoragePrefEnabled: boolean
  isPremiumModel: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  suggestionStatus: keyof typeof Mojom.SuggestionGenerationStatus
  isMobile: boolean
  isHistoryEnabled: boolean
  isAIChatAgentProfileFeatureEnabled: boolean
  isAIChatAgentProfile: boolean
  isStandalone: boolean
  isDefaultConversation: boolean
  shouldShowLongConversationInfo: boolean
  shouldShowLongPageWarning: boolean
  shouldShowLongVisualContentWarning: boolean
  totalTokens: number
  trimmedTokens: number
  isGenerating: boolean
  attachmentsDialog: 'tabs' | 'bookmarks' | 'history' | null
  isNewConversation: boolean
  generatedUrlToBeOpened: Url | undefined
  ratingTurnUuid: { isLiked: boolean; turnUuid: string } | undefined
  isTemporaryChat: boolean
  isDragActive: boolean
  isDragOver: boolean
  useMemoryHistory: boolean
  skillDialog: Mojom.Skill | null
  selectedActionType: Mojom.ActionType | undefined
  selectedSkill: Mojom.Skill | undefined
}

const args: CustomArgs = {
  initialized: true,
  inputText: [
    `Write a Star Trek poem about Data's life on board the Enterprise`,
  ],
  hasConversation: true,
  conversationListCount: CONVERSATIONS.length,
  hasSuggestedQuestions: true,
  hasAssociatedContent: true,
  editingConversationId: null,
  deletingConversationId: null,
  isFeedbackFormVisible: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: false,
  isStoragePrefEnabled: true,
  hasAcceptedAgreement: true,
  isPremiumModel: false,
  isPremiumUser: true,
  isPremiumUserDisconnected: false,
  currentErrorState: 'ConnectionIssue' satisfies keyof typeof Mojom.APIError,
  suggestionStatus:
    'None' satisfies keyof typeof Mojom.SuggestionGenerationStatus,
  model: MODELS[0].key,
  isMobile: false,
  isHistoryEnabled: true,
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
  isStandalone: false,
  isDefaultConversation: true,
  shouldShowLongConversationInfo: false,
  shouldShowLongPageWarning: false,
  shouldShowLongVisualContentWarning: false,
  totalTokens: 0,
  trimmedTokens: 0,
  isGenerating: false,
  attachmentsDialog: null,
  isNewConversation: false,
  generatedUrlToBeOpened: undefined,
  ratingTurnUuid: undefined,
  isTemporaryChat: false,
  isDragActive: false,
  isDragOver: false,
  useMemoryHistory: false,
  skillDialog: null,
  selectedActionType: undefined,
  selectedSkill: undefined,
}

export function FakeAIChatContext(props: React.PropsWithChildren<{}>) {
  const [allArgs, setAllArgs] = React.useState(args)
  const options = { args: allArgs }
  const setArgs = (newArgs: Partial<CustomArgs>) => {
    setAllArgs((current) => ({ ...current, newArgs }))
  }

  const currentError = Mojom.APIError.None
  const apiHasError = currentError !== Mojom.APIError.None
  const currentModel = MODELS.find((m) => m.displayName === options.args.model)

  const switchToBasicModel = () => {
    const nonPremiumModel = MODELS.find(
      (model) =>
        model.options.leoModelOptions?.access === Mojom.ModelAccess.BASIC,
    )
    setArgs({ model: nonPremiumModel?.key })
  }

  const [showSidebar, setShowSidebar] = React.useState(false)
  const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)

  let conversations: typeof CONVERSATIONS = []

  if (CONVERSATIONS.length <= options.args.conversationListCount) {
    conversations = conversations.concat(CONVERSATIONS)
  } else {
    const remainingConversationsCount =
      options.args.conversationListCount - conversations.length
    conversations = conversations.concat(
      CONVERSATIONS.slice(0, remainingConversationsCount),
    )
  }

  const aiChatContext: AIChatContext = {
    conversationEntriesComponent: StorybookConversationEntries,
    initialized: options.args.initialized,
    editingConversationId: options.args.editingConversationId,
    deletingConversationId: options.args.deletingConversationId,
    conversations,
    isStoragePrefEnabled: options.args.isStoragePrefEnabled,
    hasAcceptedAgreement: options.args.hasAcceptedAgreement,
    isPremiumStatusFetching: false,
    isPremiumUser: options.args.isPremiumUser,
    isPremiumUserDisconnected: options.args.isPremiumUserDisconnected,
    isStorageNoticeDismissed: options.args.isStorageNoticeDismissed,
    canShowPremiumPrompt: options.args.canShowPremiumPrompt,
    isMobile: options.args.isMobile,
    isHistoryFeatureEnabled: options.args.isHistoryEnabled,
    isAIChatAgentProfileFeatureEnabled:
      options.args.isAIChatAgentProfileFeatureEnabled,
    isAIChatAgentProfile: options.args.isAIChatAgentProfile,
    isStandalone: options.args.isStandalone,
    skills: [
      {
        id: 'translate-mode',
        shortcut: 'translate',
        prompt: 'Translate the following text to English',
        model: 'claude-3-haiku',
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt(Date.now() * 1000) },
      },
      {
        id: 'simplify-mode',
        shortcut: 'simplify',
        prompt: 'Simplify the following concept in simple terms',
        model: undefined,
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt((Date.now() - 86400000) * 1000) },
      },
      {
        id: 'summarize-mode',
        shortcut: 'summarize',
        prompt: 'Summarize the following content in bullet points',
        model: undefined,
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt((Date.now() - 3600000) * 1000) },
      },
    ],
    actionList: ACTIONS_LIST,
    tabs: [
      {
        id: 1,
        contentId: 1,
        url: { url: 'https://www.example.com' },
        title: 'Example',
      },
      {
        id: 2,
        contentId: 2,
        url: { url: 'https://topos.nz' },
        title: 'NZ Topo',
      },
      {
        id: 3,
        contentId: 3,
        url: { url: 'https://brave.com' },
        title: 'Brave',
      },
      {
        id: 4,
        contentId: 4,
        url: { url: 'https://search.brave.com' },
        title: 'Brave Search',
      },
    ],
    getPluralString: () => Promise.resolve(''),
    goPremium: () => {},
    managePremium: () => {},
    handleAgreeClick: () => {},
    enableStoragePref: () => {},
    dismissStorageNotice: () => {},
    dismissPremiumPrompt: () => {},
    userRefreshPremiumSession: () => {},
    openAIChatAgentProfile: () => {},
    setEditingConversationId: (id: string | null) =>
      setArgs({ editingConversationId: id }),
    setDeletingConversationId: (id: string | null) =>
      setArgs({ deletingConversationId: id }),
    skillDialog: options.args.skillDialog,
    setSkillDialog: () => {},
    showSidebar: showSidebar,
    toggleSidebar: () => setShowSidebar((s) => !s),
    getBookmarks: async () => [
      {
        id: BigInt(1),
        url: { url: 'https://www.example.com' },
        title: 'Example',
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt(Date.now() * 1000) },
      },
      {
        id: BigInt(2),
        url: { url: 'https://topos.nz' },
        title: 'NZ Topo',
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt(Date.now() * 1000) },
      },
    ],
    getHistory: async () => [
      {
        id: BigInt(1),
        url: { url: 'https://w3.org' },
        title: 'W3',
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt(Date.now() * 1000) },
      },
      {
        id: BigInt(2),
        url: { url: 'https://readr.nz' },
        title: 'RSS Reader',
        createdTime: { internalValue: BigInt(Date.now() * 1000) },
        lastUsed: { internalValue: BigInt(Date.now() * 1000) },
      },
    ],
  }

  const activeChatContext: SelectedChatDetails = {
    selectedConversationId: CONVERSATIONS[0].uuid,
    updateSelectedConversationId: () => {},
    callbackRouter: undefined!,
    conversationHandler: undefined!,
    createNewConversation: () => {},
    isTabAssociated: options.args.isDefaultConversation,
  }

  const inputText = options.args.inputText

  const conversationContext: ConversationContext = {
    historyInitialized: true,
    conversationUuid: options.args.isNewConversation
      ? 'new-conversation'
      : CONVERSATIONS[1].uuid,
    conversationHistory: [],
    associatedContentInfo: [],
    allModels: MODELS,
    currentModel,
    suggestedQuestions: [],
    isGenerating: options.args.isGenerating,
    suggestionStatus:
      Mojom.SuggestionGenerationStatus[options.args.suggestionStatus],
    currentError,
    apiHasError,
    isFeedbackFormVisible: options.args.isFeedbackFormVisible,
    shouldDisableUserInput: false,
    shouldShowLongPageWarning: options.args.shouldShowLongPageWarning,
    shouldShowLongConversationInfo: options.args.shouldShowLongConversationInfo,
    inputText,
    selectedActionType: options.args.selectedActionType,
    selectedSkill: options.args.selectedSkill,
    isToolsMenuOpen,
    isCurrentModelLeo: true,
    isCharLimitApproaching: inputText.length > 64,
    isCharLimitExceeded: inputText.length > 70,
    inputTextCharCountDisplay: `${inputText.length} / 70`,
    pendingMessageFiles: [],
    generatedUrlToBeOpened: options.args.generatedUrlToBeOpened,
    ratingTurnUuid: options.args.ratingTurnUuid,
    isUploadingFiles: false,
    isTemporaryChat: options.args.isTemporaryChat,
    setInputText: (content) => setArgs({ inputText: content }),
    setCurrentModel: () => {},
    switchToBasicModel,
    generateSuggestedQuestions: () => {},
    dismissLongConversationInfo: () => {},
    retryAPIRequest: () => {},
    handleResetError: () => {},
    handleStopGenerating: async () => {},
    submitInputTextToAPI: () => {},
    resetSelectedActionType: () => setArgs({ selectedActionType: undefined }),
    handleActionTypeClick: (actionType: Mojom.ActionType) => {
      const update: Partial<CustomArgs> = {
        selectedActionType: actionType,
        selectedSkill: undefined,
      }

      const content = stringifyContent(options.args.inputText)
      if (content.startsWith('/')) {
        update.inputText = ['']
      }

      setArgs(update)
      setIsToolsMenuOpen(false)
    },
    handleSkillClick: (skill: any) => {
      setArgs({
        selectedSkill: skill,
        selectedActionType: undefined,
        inputText: [
          { type: 'skill', id: skill.shortcut, text: `/${skill.shortcut}` },
        ],
      })
      setIsToolsMenuOpen(false)
    },
    handleSkillEdit: () => {},
    setIsToolsMenuOpen,
    handleFeedbackFormCancel: () => {},
    handleFeedbackFormSubmit: () => Promise.resolve(),
    setAttachmentsDialog: (attachmentsDialog) => setArgs({ attachmentsDialog }),
    attachmentsDialog: options.args.attachmentsDialog,
    removeFile: () => {},
    uploadFile: () => {},
    getScreenshots: () => {},
    setGeneratedUrlToBeOpened: (url?: Url) =>
      setArgs({ generatedUrlToBeOpened: url }),
    setIgnoreExternalLinkWarning: () => {},
    handleCloseRateMessagePrivacyModal: () =>
      setArgs({ ratingTurnUuid: undefined }),
    handleRateMessage: () => Promise.resolve(),
    setTemporary: (temporary: boolean) => {
      setArgs({ isTemporaryChat: temporary })
    },
    disassociateContent: () => {},
    isDragActive: options.args.isDragActive,
    isDragOver: options.args.isDragOver,
    clearDragState: () => {},
    attachImages: (images: Mojom.UploadedFile[]) => {},
    unassociatedTabs: aiChatContext.tabs,
    associateDefaultContent: async () => {},
  }

  return (
    <AIChatReactContext.Provider value={aiChatContext}>
      <ActiveChatContext.Provider value={activeChatContext}>
        <ConversationReactContext.Provider value={conversationContext}>
          {props.children}
        </ConversationReactContext.Provider>
      </ActiveChatContext.Provider>
    </AIChatReactContext.Provider>
  )
}
