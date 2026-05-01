/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../../common/strings'

import * as React from 'react'
import { useArgs } from '@storybook/preview-api'
import { Meta, StoryObj } from '@storybook/react'
import '@brave/leo/tokens/css/variables.css'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { InferControlsFromArgs } from '../../../../../.storybook/utils'
import * as Mojom from '../../common/mojom'
import FullPage from '../components/full_page'
import Loading from '../components/loading'
import Main from '../components/main'
import ACTIONS_LIST from './story_utils/actions'
import styles from './style.module.scss'
import StorybookConversationEntries from './story_utils/ConversationEntries'
import UntrustedMockContext from '../../untrusted_conversation_frame/mock_untrusted_conversation_context'
import { Content } from '../components/input_box/editable_content'
import { MockContext } from '../state/mock_context'
import {
  ActiveChatContext,
  SelectedChatDetails,
} from '../state/active_chat_context'

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
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    isSuggestedModel: true,
    isNearModel: false,
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        description: '',
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
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [
      Mojom.ConversationCapability.CHAT,
      Mojom.ConversationCapability.CONTENT_AGENT,
    ],
    isSuggestedModel: true,
    isNearModel: false,
    options: {
      leoModelOptions: {
        name: 'model-two-premium',
        displayMaker: 'Company',
        description: '',
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
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    isSuggestedModel: false,
    isNearModel: false,
    options: {
      leoModelOptions: {
        name: 'model-three-freemium',
        displayMaker: 'Company',
        description: '',
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
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    isSuggestedModel: false,
    isNearModel: false,
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

const SAMPLE_QUESTIONS = [
  'Summarize this article',
  'What was the score?',
  'Any injuries?',
  'Why did google executives disregard this character in the company?',
]

const ASSOCIATED_CONTENT: Mojom.AssociatedContent = {
  uuid: 'uuid',
  contentType: Mojom.ContentType.PageContent,
  title: 'Tiny Tweaks to Neurons Can Rewire Animal Motion',
  contentUsedPercentage: 40,
  url: {
    url: 'https://www.example.com/areallylongurlthatwillbetruncatedintheinputbox',
  },
  contentId: 1,
  conversationTurnUuid: 'turn-uuid',
}

const SAMPLE_SKILLS: Mojom.Skill[] = [
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
]

const SAMPLE_TABS: Mojom.TabData[] = [
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
]

const SAMPLE_BOOKMARKS: Mojom.Bookmark[] = [
  {
    id: BigInt(1),
    url: { url: 'https://www.example.com' },
    title: 'Example',
  },
  {
    id: BigInt(2),
    url: { url: 'https://topos.nz' },
    title: 'NZ Topo',
  },
]

const SAMPLE_HISTORY_ENTRIES: Mojom.HistoryEntry[] = [
  {
    id: BigInt(1),
    url: { url: 'https://w3.org' },
    title: 'W3',
  },
  {
    id: BigInt(2),
    url: { url: 'https://readr.nz' },
    title: 'RSS Reader',
  },
]

const CONVERSATION_TYPES = [
  'ALL',
  'MEMORY',
  'SEARCH',
  'SEARCHING',
  'WEATHER',
  'CODE_EXECUTION',
  'MULTI_TOOL_MULTI_TURN',
  'MULTI_TOOL_MULTI_TURN_IN_PROGRESS',
  'NONE',
] as const

type CustomArgs = {
  conversation_content: (typeof CONVERSATION_TYPES)[number]
  initialized: boolean
  currentErrorState: keyof typeof Mojom.APIError
  toolUseTaskState: keyof typeof Mojom.TaskState
  model: string
  inputText: Content
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
  isToolExecuting: boolean
  attachmentsDialog: 'tabs' | 'bookmarks' | 'history' | null
  isNewConversation: boolean
  generatedUrlToBeOpened: Url | undefined
  ratingTurnUuid: { isLiked: boolean; turnUuid: string } | undefined
  isTemporaryChat: boolean
  isDragActive: boolean
  isDragOver: boolean
  skillDialog: Mojom.Skill | null
  selectedActionType: Mojom.ActionType | undefined
  selectedSkill: Mojom.Skill | undefined
}

const args: CustomArgs = {
  conversation_content: 'ALL',
  initialized: true,
  inputText: [
    `Write a Star Trek poem about Data's life on board the Enterprise`,
  ],
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
  toolUseTaskState: 'kNone' satisfies keyof typeof Mojom.TaskState,
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
  isToolExecuting: false,
  attachmentsDialog: null,
  isNewConversation: false,
  generatedUrlToBeOpened: undefined,
  ratingTurnUuid: undefined,
  isTemporaryChat: false,
  isDragActive: false,
  isDragOver: false,
  skillDialog: null,
  selectedActionType: undefined,
  selectedSkill: undefined,
}

const meta: Meta<CustomArgs> = {
  title: 'AI Chat/Conversation',
  parameters: {
    layout: 'centered',
  },
  argTypes: {
    ...InferControlsFromArgs(args),
    conversation_content: {
      options: CONVERSATION_TYPES,
      control: { type: 'select' },
    },
    currentErrorState: {
      options: getKeysForMojomEnum(Mojom.APIError),
      control: { type: 'select' },
    },
    toolUseTaskState: {
      options: getKeysForMojomEnum(Mojom.TaskState),
      control: { type: 'select' },
    },
    suggestionStatus: {
      options: getKeysForMojomEnum(Mojom.SuggestionGenerationStatus),
      control: { type: 'select' },
    },
    model: {
      options: MODELS.map((model) => model.displayName),
      control: { type: 'select' },
    },
    conversationListCount: {
      control: { type: 'number' },
    },
    deletingConversationId: {
      options: CONVERSATIONS.map((conversation) => conversation.uuid),
      control: { type: 'select' },
    },
    generatedUrlToBeOpened: {
      options: [{ url: 'https://www.example.com' }],
      control: { type: 'select' },
    },
    ratingTurnUuid: {
      options: [{ isLiked: true, turnUuid: 'turn-uuid' }],
      control: { type: 'select' },
    },
  },
  args,
  decorators: [
    (Story, options) => {
      const [args, setArgs] = useArgs<CustomArgs>()
      return (
        <StoryContext
          args={args}
          setArgs={setArgs}
        >
          <Story />
        </StoryContext>
      )
    },
  ],
}

async function getConversationContent(
  conversationType: (typeof CONVERSATION_TYPES)[number],
): Promise<Mojom.ConversationTurn[]> {
  switch (conversationType) {
    case 'ALL':
      return (await import('./conversations/all')).default
    case 'MEMORY':
      return (await import('./conversations/memory')).default
    case 'SEARCH':
      return (await import('./conversations/search')).default
    case 'SEARCHING':
      return (await import('./conversations/searching')).default
    case 'WEATHER':
      return (await import('./conversations/weather')).default
    case 'CODE_EXECUTION':
      return (await import('./conversations/code_execution')).default
    case 'MULTI_TOOL_MULTI_TURN':
      return (await import('./conversations/multi_tool_multi_turn')).default
    case 'MULTI_TOOL_MULTI_TURN_IN_PROGRESS':
      return (await import('./conversations/multi_tool_multi_turn')).InProgress
  }

  return []
}

function StoryContext(
  props: React.PropsWithChildren<{
    args: CustomArgs
    setArgs: (newArgs: Partial<CustomArgs>) => void
  }>,
) {
  const { args } = props
  // Ref holds current args - for inside function lookup
  const argsRef = React.useRef(args)
  argsRef.current = args

  // Compute derived values from args for UntrustedConversationContext
  const getAssociatedContent = () =>
    argsRef.current.hasAssociatedContent
      ? ASSOCIATED_CONTENT
      : new Mojom.AssociatedContent()

  const activeChatContext: SelectedChatDetails = {
    // api instance not needed when we're providing ConversationContext directly
    api: undefined!,
    selectedConversationId: CONVERSATIONS[0].uuid,
    updateSelectedConversationId: () => {},
    createNewConversation: () => {},
    isTabAssociated: argsRef.current.isDefaultConversation,
  }

  const currentError = Mojom.APIError[args.currentErrorState]
  const currentModel =
    MODELS.find((m) => m.displayName === argsRef.current.model) ?? MODELS[0]

  const getConversationHistory = () =>
    getConversationContent(argsRef.current.conversation_content)

  return (
    <MockContext
      service={{
        getConversations: () => {
          const count = argsRef.current.conversationListCount
          const conversations =
            CONVERSATIONS.length <= count
              ? CONVERSATIONS
              : CONVERSATIONS.slice(0, count)
          return Promise.resolve({ conversations })
        },
        getActionMenuList: () => Promise.resolve({ actionList: ACTIONS_LIST }),
        getSkills: () => Promise.resolve({ skills: SAMPLE_SKILLS }),
        getPremiumStatus: () =>
          Promise.resolve({
            status: argsRef.current.isPremiumUser
              ? argsRef.current.isPremiumUserDisconnected
                ? Mojom.PremiumStatus.ActiveDisconnected
                : Mojom.PremiumStatus.Active
              : Mojom.PremiumStatus.Inactive,
            info: null,
          }),
      }}
      bookmarksService={{
        getBookmarks: () => Promise.resolve({ bookmarks: SAMPLE_BOOKMARKS }),
      }}
      historyService={{
        getHistory: () =>
          Promise.resolve({
            history: argsRef.current.isHistoryEnabled
              ? SAMPLE_HISTORY_ENTRIES
              : [],
          }),
      }}
      initialState={React.useMemo(
        () => ({
          tabs: SAMPLE_TABS,
          isStandalone: args.isStandalone,
          serviceState: {
            hasAcceptedAgreement: args.hasAcceptedAgreement,
            isStoragePrefEnabled: args.isStoragePrefEnabled,
            isStorageNoticeDismissed: args.isStorageNoticeDismissed,
            canShowPremiumPrompt: args.canShowPremiumPrompt,
          },
        }),
        [args],
      )}
      conversationHandler={{
        getState: () => {
          return Promise.resolve({
            conversationState: {
              conversationUuid: argsRef.current.isNewConversation
                ? 'new-conversation'
                : CONVERSATIONS[1].uuid,
              isRequestInProgress: argsRef.current.isGenerating,
              currentModelKey: currentModel.key,
              defaultModelKey: MODELS[0].key,
              allModels: MODELS,
              suggestedQuestions: argsRef.current.hasSuggestedQuestions
                ? SAMPLE_QUESTIONS
                : argsRef.current.hasAssociatedContent
                  ? [SAMPLE_QUESTIONS[0]]
                  : [],
              suggestionStatus:
                Mojom.SuggestionGenerationStatus[
                  argsRef.current.suggestionStatus
                ],
              associatedContent: [
                argsRef.current.hasAssociatedContent
                  ? ASSOCIATED_CONTENT
                  : new Mojom.AssociatedContent(),
              ],
              error: currentError,
              temporary: argsRef.current.isTemporaryChat,
              toolUseTaskState:
                Mojom.TaskState[argsRef.current.toolUseTaskState],
            },
          })
        },
        getConversationHistory: async () => ({
          conversationHistory: await getConversationHistory(),
        }),
      }}
      conversationProps={{
        selectedConversationId: activeChatContext.selectedConversationId,
        isTabAssociated: activeChatContext.isTabAssociated,
      }}
      // Overrides for values that come from internal hooks (useState, etc.)
      // and can't be controlled via API mocks

      aiChatOverrides={{
        conversationEntriesComponent: StorybookConversationEntries,
        editingConversationId: args.editingConversationId,
        deletingConversationId: args.deletingConversationId,
        initialized: args.initialized,
        isAIChatAgentProfileFeatureEnabled:
          args.isAIChatAgentProfileFeatureEnabled,
        isAIChatAgentProfile: args.isAIChatAgentProfile,
        isMobile: args.isMobile,
        isHistoryFeatureEnabled: args.isHistoryEnabled,
        skillDialog: args.skillDialog,
      }}
      conversationOverrides={{
        isFeedbackFormVisible: args.isFeedbackFormVisible,
        ratingTurnUuid: args.ratingTurnUuid,
        inputText: args.inputText,
        selectedActionType: args.selectedActionType,
        selectedSkill: args.selectedSkill,
        attachmentsDialog: args.attachmentsDialog,
        isDragActive: args.isDragActive,
        isDragOver: args.isDragOver,
        generatedUrlToBeOpened: args.generatedUrlToBeOpened,
      }}
      deps={[...Object.values(args)]}
    >
      <ActiveChatContext.Provider value={activeChatContext}>
        <UntrustedMockContext
          conversationHandler={{
            getConversationHistory: async () => ({
              conversationHistory: await getConversationHistory(),
            }),
          }}
          uiHandler={{
            hasMemory: (memory: string) => {
              // Return false for the "undone" example to show undone state
              if (memory === 'Likes cats') {
                return Promise.resolve({ exists: false })
              }
              // Return true for others to show success state
              return Promise.resolve({ exists: true })
            },
          }}
          initialState={{
            conversationEntriesState: {
              isGenerating: args.isGenerating,
              isToolExecuting: args.isToolExecuting,
              toolUseTaskState: Mojom.TaskState[args.toolUseTaskState],
              isLeoModel: true,
              contentUsedPercentage: args.shouldShowLongPageWarning ? 48 : 100,
              visualContentUsedPercentage:
                args.shouldShowLongVisualContentWarning ? 75 : undefined,
              totalTokens: BigInt(args.totalTokens),
              trimmedTokens: BigInt(args.trimmedTokens),
              canSubmitUserEntries: currentError === Mojom.APIError.None,
              allModels: MODELS,
              currentModelKey: currentModel?.key ?? '',
              conversationCapabilities: [
                Mojom.ConversationCapability.CONTENT_AGENT,
              ],
            },
          }}
          overrides={{
            isMobile: args.isMobile,
            isHistoryFeatureEnabled: args.isHistoryEnabled,
            associatedContent: [getAssociatedContent()],
          }}
          deps={[...Object.values(args)]}
        >
          {props.children}
        </UntrustedMockContext>
      </ActiveChatContext.Provider>
    </MockContext>
  )
}

export default meta

type Story = StoryObj<CustomArgs>

function GetPanelStory(withArgs: Partial<CustomArgs> = args): Story {
  return {
    args: withArgs,
    render: () => {
      return (
        <div className={styles.container}>
          <Main />
        </div>
      )
    },
  }
}

export const _Panel = GetPanelStory({
  conversation_content: 'ALL',
})

export const _WithSearch = GetPanelStory({
  conversation_content: 'SEARCH',
})

export const _WithSearching = GetPanelStory({
  conversation_content: 'SEARCHING',
  isGenerating: true,
})

export const _WithMemory = GetPanelStory({
  conversation_content: 'MEMORY',
})

export const _WithCodeExecution = GetPanelStory({
  conversation_content: 'CODE_EXECUTION',
})

export const _WithMultipleToolsMultipleTurns = GetPanelStory({
  conversation_content: 'MULTI_TOOL_MULTI_TURN',
})

export const _WithMultipleToolsMultipleTurnsInProgress = GetPanelStory({
  conversation_content: 'MULTI_TOOL_MULTI_TURN_IN_PROGRESS',
  isGenerating: true,
  isToolExecuting: true,
})

export const _WithWeather = GetPanelStory({
  conversation_content: 'WEATHER',
})

export const _FullPage = {
  args: {
    isStandalone: true,
    isDefaultConversation: false,
  },
  render: () => {
    return (
      <div className={styles.containerFull}>
        <FullPage />
      </div>
    )
  },
}

export const _NewFullpageConversation = {
  args: {
    isNewConversation: true,
    isStandalone: true,
    conversationUuid: undefined,
    conversation_content: 'NONE',
    hasSiteInfo: false,
  },
  render: () => {
    return (
      <div className={styles.container}>
        <FullPage />
      </div>
    )
  },
}

export const _Loading = {
  args: {
    initialized: false,
  },
  render: () => {
    return (
      <div className={styles.container}>
        <Loading />
      </div>
    )
  },
}
