/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useArgs } from '@storybook/preview-api'
import { Meta, StoryObj } from '@storybook/react'
import '@brave/leo/tokens/css/variables.css'
import '$web-components/app.global.scss'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { InferControlsFromArgs } from '../../../../../.storybook/utils'
import * as Mojom from '../../common/mojom'
import { ActiveChatContext, SelectedChatDetails } from '../state/active_chat_context'
import { AIChatContext, AIChatReactContext, useIsSmall } from '../state/ai_chat_context'
import { ConversationContext, ConversationReactContext } from '../state/conversation_context'
import FeedbackForm from '../components/feedback_form'
import FullPage from '../components/full_page'
import Loading from '../components/loading'
import Main from '../components/main'
import './story_utils/locale'
import ACTIONS_LIST from './story_utils/actions'
import styles from './style.module.scss'
import StorybookConversationEntries from './story_utils/ConversationEntries'
import { UntrustedConversationContext, UntrustedConversationReactContext } from '../../untrusted_conversation_frame/untrusted_conversation_context'
import ErrorConnection from '../components/alerts/error_connection'
import ErrorConversationEnd from '../components/alerts/error_conversation_end'
import ErrorInvalidAPIKey from '../components/alerts/error_invalid_api_key'
import ErrorInvalidEndpointURL from '../components/alerts/error_invalid_endpoint_url'
import ErrorRateLimit from '../components/alerts/error_rate_limit'
import ErrorServiceOverloaded from '../components/alerts/error_service_overloaded'
import LongConversationInfo from '../components/alerts/long_conversation_info'
import WarningPremiumDisconnected from '../components/alerts/warning_premium_disconnected'
import Attachments from '../components/attachments'

const eventTemplate: Mojom.ConversationEntryEvent = {
  completionEvent: undefined,
  pageContentRefineEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  selectedLanguageEvent: undefined,
  conversationTitleEvent: undefined,
  sourcesEvent: undefined,
  contentReceiptEvent: undefined
}

function getCompletionEvent(text: string): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    completionEvent: { completion: text }
  }
}

function getSearchEvent(queries: string[]): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    searchQueriesEvent: { searchQueries: queries }
  }
}

function getSearchStatusEvent(): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    searchStatusEvent: { isSearching: true }
  }
}

function getWebSourcesEvent(sources: Mojom.WebSource[]): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    sourcesEvent: { sources }
  }
}

function getPageContentRefineEvent(): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    pageContentRefineEvent: { isRefining: true }
  }
}

const CONVERSATIONS: Mojom.Conversation[] = [
  {
    title: 'Star Trek Poem',
    uuid: '1',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000000') },
    associatedContent: undefined,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
  },
  {
    title: 'Sorting C++ vectors is hard especially when you have to have a very long title for your conversation to test text clipping or wrapping',
    uuid: '2',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000001') },
    associatedContent: undefined,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
  },
  {
    title: '',
    uuid: '3',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000002') },
    associatedContent: undefined,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
  }
]

const HISTORY: Mojom.ConversationTurn[] = [
  {
    uuid: undefined,
    text: 'Summarize this page',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SUMMARIZE_PAGE,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('The ways that animals move are just about as myriad as the animal kingdom itself. They walk, run, swim, crawl, fly and slither — and within each of those categories lies a tremendous number of subtly different movement types. A seagull and a *hummingbird* both have wings, but otherwise their flight techniques and abilities are poles apart. Orcas and **piranhas** both have tails, but they accomplish very different types of swimming. Even a human walking or running is moving their body in fundamentally different ways.')],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'What is pointer compression?\n...and how does it work?\n    - tell me something interesting',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent(`## How We Created an Accessible, Scalable Color Palette\n\nDuring the latter part of 2021, I reflected on the challenges we were facing at Modern Health. One recurring problem that stood out was our struggle to create new products with an unstructured color palette. This resulted in poor [communication](https://www.google.com) between designers and developers, an inconsistent product brand, and increasing accessibility problems.\n\n1. Inclusivity: our palette provides easy ways to ensure our product uses accessible contrasts.\n 2. Efficiency: our palette is diverse enough for our current and future product design, yet values are still predictable and constrained.\n 3. Reusability: our palette is on-brand but versatile. There are very few one-offs that fall outside the palette.\n\n This article shares the process I followed to apply these principles to develop a more adaptable color palette that prioritizes accessibility and is built to scale into all of our future product **design** needs.`)],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getPageContentRefineEvent()],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'What is taylor series?',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.')],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Write a hello world program in c++',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent("Hello! As a helpful and respectful AI assistant, I'd be happy to assist you with your question. However, I'm a text-based AI and cannot provide code in a specific programming language like C++. Instead, I can offer a brief explanation of how to write a \"hello world\" program in C++.\n\nTo write a \"hello world\" program in C++, you can use the following code:\n\n```c++\n#include <iostream>\n\nint main() {\n    std::cout << \"Hello, world!\" << std::endl;\n    return 0;\n}\n```\nThis code will print \"Hello, world!\" and uses `iostream` std library. If you have any further questions or need more information, please don't hesitate to ask!")],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Summarize this excerpt',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SUMMARIZE_SELECTED_TEXT,
    prompt: undefined,
    selectedText: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.',
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.')],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Shorten this selected text',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SHORTEN,
    prompt: undefined,
    selectedText: 'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [
      getSearchStatusEvent(),
      getSearchEvent(['pointer compression', 'c++ language specification']),
      getCompletionEvent('[1]:https://www.example.com\n[2]:https://lttstore.com\n[3]:https://www.tesla.com/modely\n[Pointer compression](https://www.example.com) is a [memory](https://brave.com/wont-show-as-link) optimization technique.[1][3]'),
      getWebSourcesEvent([
        { url: { url: 'https://www.example.com' }, title: 'Pointer Compression', faviconUrl: { url: 'https://www.example.com/favicon.ico' } },
        { title: 'LTT Store', faviconUrl: { url: 'https://lttstore.com/favicon.ico' }, url: { url: 'https://lttstore.com' } },
        { title: 'Tesla Model Y', faviconUrl: { url: 'https://www.tesla.com/favicon.ico' }, url: { url: 'https://www.tesla.com/modely' } }
      ])
    ],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Will an LTT store backpack fit in a Tesla Model Y frunk?',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SHORTEN,
    prompt: undefined,
    selectedText: '',
    edits: [{
      uuid: undefined,
      text: 'Will it fit in a Tesla Model Y frunk?',
      characterType: Mojom.CharacterType.HUMAN,
      actionType: Mojom.ActionType.SHORTEN,
      prompt: undefined,
      selectedText: '',
      createdTime: { internalValue: BigInt('13278618001000000') },
      edits: [],
      events: [],
      uploadedImages : [],
      fromBraveSearchSERP: false
    }],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getSearchStatusEvent(), getSearchEvent(['LTT store backpack dimensions', 'Tesla Model Y frunk dimensions'])],
    uploadedImages : [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'What is this image?',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedImages : [
      { filename: 'lion.png', filesize: 128,
        imageData: Array.from(new Uint8Array(128)) }
    ],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('It is a lion!')],
    uploadedImages : [],
    fromBraveSearchSERP: false
  }
]

const MODELS: Mojom.Model[] = [
  {
    key: '1',
    displayName: 'Model One',
    visionSupport: false,
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        engineType: Mojom.ModelEngineType.LLAMA_REMOTE,
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '2',
    displayName: 'Model Two',
    visionSupport: true,
    options: {
      leoModelOptions: {
        name: 'model-two-premium',
        displayMaker: 'Company',
        engineType: Mojom.ModelEngineType.LLAMA_REMOTE,
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '3',
    displayName: 'Model Three',
    visionSupport: false,
    options: {
      leoModelOptions: {
        name: 'model-three-freemium',
        displayMaker: 'Company',
        engineType: Mojom.ModelEngineType.LLAMA_REMOTE,
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '4',
    displayName: 'Microsoft Phi-3',
    visionSupport: false,
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
          'You are a helpful AI assistant who wishes not to be ' +
          'associated with a character named Clippy.',
        endpoint: { url: 'https://example.com' },
        apiKey: '123456',
      },
    }
  },
]

const SAMPLE_QUESTIONS = [
  'Summarize this article',
  'What was the score?',
  'Any injuries?',
  'Why did google executives disregard this character in the company?'
]

const ASSOCIATED_CONTENT: Mojom.AssociatedContent = {
  uuid: 'uuid',
  contentType: Mojom.ContentType.PageContent,
  title: 'Tiny Tweaks to Neurons Can Rewire Animal Motion',
  contentUsedPercentage: 40,
  url: { url: 'https://www.example.com/a' },
  isContentRefined: false,
  contentId: 1,
}

type CustomArgs = {
  initialized: boolean
  currentErrorState: keyof typeof Mojom.APIError
  model: string
  inputText: string
  hasConversation: boolean
  editingConversationId: string | null
  deletingConversationId: string | null
  visibleConversationListCount: number
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
  isStandalone: boolean
  isDefaultConversation: boolean
  shouldShowLongConversationInfo: boolean
  shouldShowLongPageWarning: boolean
  shouldShowRefinedWarning: boolean
  totalTokens: number
  trimmedTokens: number
  isGenerating: boolean
  showAttachments: boolean
  isNewConversation: boolean
  generatedUrlToBeOpened: Url | undefined
}

const args: CustomArgs = {
  initialized: true,
  inputText: `Write a Star Trek poem about Data's life on board the Enterprise`,
  hasConversation: true,
  visibleConversationListCount: CONVERSATIONS.length,
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
  suggestionStatus: 'None' satisfies keyof typeof Mojom.SuggestionGenerationStatus,
  model: MODELS[0].key,
  isMobile: false,
  isHistoryEnabled: true,
  isStandalone: false,
  isDefaultConversation: true,
  shouldShowLongConversationInfo: false,
  shouldShowLongPageWarning: false,
  shouldShowRefinedWarning: false,
  totalTokens: 0,
  trimmedTokens: 0,
  isGenerating: false,
  showAttachments: true,
  isNewConversation: false,
  generatedUrlToBeOpened: undefined
}

const meta: Meta<CustomArgs> = {
  title: 'Chat/Chat',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    ...InferControlsFromArgs(args),
    currentErrorState: {
      options: getKeysForMojomEnum(Mojom.APIError),
      control: { type: 'select' }
    },
    suggestionStatus: {
      options: getKeysForMojomEnum(Mojom.SuggestionGenerationStatus),
      control: { type: 'select' }
    },
    model: {
      options: MODELS.map(model => model.displayName),
      control: { type: 'select' }
    },
    visibleConversationListCount: {
      control: { type: 'number' }
    },
    deletingConversationId: {
      options: CONVERSATIONS.map(conversation => conversation.uuid),
      control: { type: 'select' }
    },
    generatedUrlToBeOpened: {
      options: [{ url: 'https://www.example.com' }],
      control: { type: 'select' }
    }
  },
  args,
  decorators: [
    (Story, options) => {
      const [, setArgs] = useArgs()
      return (
        <StoryContext args={options.args} setArgs={setArgs}>
          <Story />
        </StoryContext>
      )
    }
  ]
}

function StoryContext(props: React.PropsWithChildren<{ args: CustomArgs, setArgs: (newArgs: Partial<CustomArgs>) => void }>) {
  const isSmall = useIsSmall()

  const options = { args: props.args }
  const { setArgs } = props

  const associatedContent = options.args.hasAssociatedContent ? ASSOCIATED_CONTENT : new Mojom.AssociatedContent()
  const suggestedQuestions = options.args.hasSuggestedQuestions
    ? SAMPLE_QUESTIONS
    : associatedContent
      ? [SAMPLE_QUESTIONS[0]]
      : []

  const currentError = Mojom.APIError[options.args.currentErrorState]
  const apiHasError = currentError !== Mojom.APIError.None
  const currentModel = MODELS.find(m => m.displayName === options.args.model)

  const switchToBasicModel = () => {
    const nonPremiumModel = MODELS.find(model => model.options.leoModelOptions?.access === Mojom.ModelAccess.BASIC)
    setArgs({ model: nonPremiumModel?.key })
  }

  const setInputText = (inputText: string) => {
    setArgs({ inputText })
  }

  const [showSidebar, setShowSidebar] = React.useState(isSmall)

  let visibleConversations: typeof CONVERSATIONS = []
  for (let i = 0; i < Math.floor(options.args.visibleConversationListCount / CONVERSATIONS.length); i++) {
    visibleConversations = visibleConversations.concat(CONVERSATIONS)
  }
  const remainingConversationsCount = options.args.visibleConversationListCount % CONVERSATIONS.length
  visibleConversations = visibleConversations.concat(CONVERSATIONS.slice(0, remainingConversationsCount))

  const aiChatContext: AIChatContext = {
    conversationEntriesComponent: StorybookConversationEntries,
    initialized: options.args.initialized,
    editingConversationId: options.args.editingConversationId,
    deletingConversationId: options.args.deletingConversationId,
    visibleConversations,
    isStoragePrefEnabled: options.args.isStoragePrefEnabled,
    hasAcceptedAgreement: options.args.hasAcceptedAgreement,
    isPremiumStatusFetching: false,
    isPremiumUser: options.args.isPremiumUser,
    isPremiumUserDisconnected: options.args.isPremiumUserDisconnected,
    isStorageNoticeDismissed: options.args.isStorageNoticeDismissed,
    canShowPremiumPrompt: options.args.canShowPremiumPrompt,
    isMobile: options.args.isMobile,
    isHistoryFeatureEnabled: options.args.isHistoryEnabled,
    isStandalone: options.args.isStandalone,
    allActions: ACTIONS_LIST,
    tabs: [{
      id: 1,
      contentId: 1,
      url: { url: 'https://www.example.com' },
      title: 'Example',
    }, {
      id: 2,
      contentId: 2,
      url: { url: 'https://topos.nz' },
      title: 'NZ Topo',
    }, {
      id: 3,
      contentId: 3,
      url: { url: 'https://brave.com' },
      title: 'Brave',
    }, {
      id: 4,
      contentId: 4,
      url: { url: 'https://search.brave.com' },
      title: 'Brave Search',
    }],
    goPremium: () => { },
    managePremium: () => { },
    handleAgreeClick: () => { },
    enableStoragePref: () => { },
    dismissStorageNotice: () => { },
    dismissPremiumPrompt: () => { },
    userRefreshPremiumSession: () => { },
    setEditingConversationId: (id: string | null) => setArgs({ editingConversationId: id }),
    setDeletingConversationId: (id: string | null) => setArgs({ deletingConversationId: id }),
    showSidebar: showSidebar,
    toggleSidebar: () => setShowSidebar(s => !s)
  }

  const activeChatContext: SelectedChatDetails = {
    selectedConversationId: CONVERSATIONS[0].uuid,
    updateSelectedConversationId: () => { },
    callbackRouter: undefined!,
    conversationHandler: undefined!,
    createNewConversation: () => { },
    isTabAssociated: options.args.isDefaultConversation
  }

  const inputText = options.args.inputText

  const conversationContext: ConversationContext = {
    historyInitialized: true,
    conversationUuid: options.args.isNewConversation
      ? 'new-conversation'
      : CONVERSATIONS[1].uuid,
    conversationHistory: options.args.hasConversation ? HISTORY : [],
    associatedContentInfo: associatedContent,
    allModels: MODELS,
    currentModel,
    suggestedQuestions,
    isGenerating: options.args.isGenerating,
    suggestionStatus: Mojom.SuggestionGenerationStatus[options.args.suggestionStatus],
    currentError,
    apiHasError,
    isFeedbackFormVisible: options.args.isFeedbackFormVisible,
    shouldDisableUserInput: false,
    shouldShowLongPageWarning: options.args.shouldShowLongPageWarning,
    shouldShowLongConversationInfo: options.args.shouldShowLongConversationInfo,
    shouldSendPageContents: !!associatedContent,
    inputText,
    actionList: ACTIONS_LIST,
    selectedActionType: undefined,
    isToolsMenuOpen: false,
    isCurrentModelLeo: true,
    isCharLimitApproaching: inputText.length > 64,
    isCharLimitExceeded: inputText.length > 70,
    inputTextCharCountDisplay: `${inputText.length} / 70`,
    pendingMessageImages: null,
    generatedUrlToBeOpened: options.args.generatedUrlToBeOpened,
    setInputText,
    setCurrentModel: () => { },
    switchToBasicModel,
    generateSuggestedQuestions: () => { },
    dismissLongConversationInfo: () => { },
    updateShouldSendPageContents: () => { },
    retryAPIRequest: () => { },
    handleResetError: () => { },
    handleStopGenerating: async () => { },
    submitInputTextToAPI: () => { },
    resetSelectedActionType: () => { },
    handleActionTypeClick: () => { },
    setIsToolsMenuOpen: () => { },
    handleFeedbackFormCancel: () => { },
    handleFeedbackFormSubmit: () => { },
    setShowAttachments: (show: boolean) => setArgs({ showAttachments: show }),
    showAttachments: options.args.showAttachments,
    removeImage: () => {},
    uploadImage: (useMediaCapture: boolean) => {},
    setGeneratedUrlToBeOpened:
      (url?: Url) => setArgs({ generatedUrlToBeOpened: url }),
    setIgnoreExternalLinkWarning: () => { }
  }

  const conversationEntriesContext: UntrustedConversationContext = {
    conversationHistory: conversationContext.conversationHistory,
    isGenerating: conversationContext.isGenerating,
    isLeoModel: conversationContext.isCurrentModelLeo,
    contentUsedPercentage: (options.args.shouldShowLongPageWarning ||
                            options.args.shouldShowRefinedWarning)
      ? 48 : 100,
    isContentRefined: options.args.shouldShowRefinedWarning,
    totalTokens: BigInt(options.args.totalTokens),
    trimmedTokens: BigInt(options.args.trimmedTokens),
    canSubmitUserEntries: !conversationContext.shouldDisableUserInput,
    isMobile: aiChatContext.isMobile
  }

  return (
    <AIChatReactContext.Provider value={aiChatContext}>
      <ActiveChatContext.Provider value={activeChatContext}>
        <ConversationReactContext.Provider value={conversationContext}>
          <UntrustedConversationReactContext.Provider value={conversationEntriesContext}>
            {props.children}
          </UntrustedConversationReactContext.Provider>
        </ConversationReactContext.Provider>
      </ActiveChatContext.Provider>
    </AIChatReactContext.Provider>
  )
}

export default meta

type Story = StoryObj<CustomArgs>

export const _Panel: Story = {
  render: (args) => {
    return (
      <div className={styles.container}>
        <Main />
      </div>
    )
  }
}

export const _Alerts = {
  render: () => {
    return (
      <div className={`${styles.container} ${styles.containerAlerts}`}>
        <ErrorConnection />
        <ErrorConversationEnd />
        <ErrorInvalidAPIKey />
        <ErrorInvalidEndpointURL />
        <ErrorRateLimit />
        <ErrorRateLimit _testIsCurrentModelLeo={false} />
        <ErrorServiceOverloaded />
        <LongConversationInfo />
        <WarningPremiumDisconnected />
      </div>
    )
  }
}

export const _FeedbackForm = {
  render: () => {
    return (
      <div className={styles.container}>
        <FeedbackForm />
      </div>
    )
  }
}

export const _FullPage = {
  args: {
    isStandalone: true,
    isDefaultConversation: false
  },
  render: () => {
    return (
      <div className={styles.containerFull}>
        <FullPage />
      </div>
    )
  }
}

export const _NewFullpageConversation = {
  args: {
    isNewConversation: true,
    isStandalone: true,
    conversationUuid: undefined,
    hasConversation: false,
    hasSiteInfo: false,
  },
  render: () => {
    return <div className={styles.container}>
      <FullPage />
    </div>
  }
}
export const _AttachmentsPanel = {
  args: {
    isStandalone: true,
    isDefaultConversation: false
  },
  render: () => {
    return (
      <div className={styles.container}>
        <Attachments />
      </div>
    )
  }
}

export const _Loading = {
  args: {
    initialized: false
  },
  render: () => {
    return (
      <div className={styles.container}>
        <Loading />
      </div>
    )
  }
}
