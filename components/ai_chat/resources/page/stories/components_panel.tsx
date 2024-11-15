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
import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import { InferControlsFromArgs } from '../../../../../.storybook/utils'
import * as mojom from '../api/'
import { AIChatContext, AIChatReactContext } from '../state/ai_chat_context'
import { ConversationContext, ConversationReactContext } from '../state/conversation_context'
import FeedbackForm from '../components/feedback_form'
import FullPage from '../components/full_page'
import Main from '../components/main'
import './locale'
import ACTIONS_LIST from './actions'
import styles from './style.module.scss'

function getCompletionEvent(text: string): mojom.ConversationEntryEvent {
  return {
    completionEvent: { completion: text },
    pageContentRefineEvent: undefined,
    searchQueriesEvent: undefined,
    searchStatusEvent: undefined,
    conversationTitleEvent: undefined,
    selectedLanguageEvent: undefined
  }
}

function getSearchEvent(queries: string[]): mojom.ConversationEntryEvent {
  return {
    completionEvent: undefined,
    pageContentRefineEvent: undefined,
    searchQueriesEvent: { searchQueries: queries },
    searchStatusEvent: undefined,
    conversationTitleEvent: undefined,
    selectedLanguageEvent: undefined
  }
}

function getSearchStatusEvent(): mojom.ConversationEntryEvent {
  return {
    completionEvent: undefined,
    pageContentRefineEvent: undefined,
    searchQueriesEvent: undefined,
    searchStatusEvent: { isSearching: true },
    selectedLanguageEvent: undefined,
    conversationTitleEvent: undefined
  }
}

function getPageContentRefineEvent(): mojom.ConversationEntryEvent {
  return {
    completionEvent: undefined,
    pageContentRefineEvent: { isRefining: true },
    searchQueriesEvent: undefined,
    searchStatusEvent: undefined,
    selectedLanguageEvent: undefined,
    conversationTitleEvent: undefined
  }
}

const associatedContentNone: mojom.SiteInfo =  {
  uuid: undefined,
  contentType: mojom.ContentType.PageContent,
  isContentAssociationPossible: false,
  contentUsedPercentage: 0,
  isContentRefined: false,
  title: undefined,
  hostname: undefined,
  url: undefined,
}

const CONVERSATIONS: mojom.Conversation[] = [
  {
    title: 'Star Trek Poem',
    uuid: '1',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000000') },
    associatedContent: associatedContentNone,
    modelKey: undefined,
  },
  {
    title: 'Sorting C++ vectors is hard especially when you have to have a very long title for your conversation to test text clipping or wrapping',
    uuid: '2',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000001') },
    associatedContent: associatedContentNone,
    modelKey: undefined,
  },
  {
    title: 'Wedding speech improvements',
    uuid: '3',
    hasContent: true,
    updatedTime: { internalValue: BigInt('13278618001000002') },
    associatedContent: associatedContentNone,
    modelKey: undefined,
  }
]

const HISTORY: mojom.ConversationTurn[] = [
  {
    uuid: undefined,
    text: 'Summarize this page',
    characterType: mojom.CharacterType.HUMAN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    actionType: mojom.ActionType.SUMMARIZE_PAGE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    actionType: mojom.ActionType.UNSPECIFIED,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('The ways that animals move are just about as myriad as the animal kingdom itself. They walk, run, swim, crawl, fly and slither — and within each of those categories lies a tremendous number of subtly different movement types. A seagull and a *hummingbird* both have wings, but otherwise their flight techniques and abilities are poles apart. Orcas and **piranhas** both have tails, but they accomplish very different types of swimming. Even a human walking or running is moving their body in fundamentally different ways.')],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'What is pointer compression?\n...and how does it work?\n    - tell me something interesting',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent(`## How We Created an Accessible, Scalable Color Palette\n\nDuring the latter part of 2021, I reflected on the challenges we were facing at Modern Health. One recurring problem that stood out was our struggle to create new products with an unstructured color palette. This resulted in poor [communication](https://www.google.com) between designers and developers, an inconsistent product brand, and increasing accessibility problems.\n\n1. Inclusivity: our palette provides easy ways to ensure our product uses accessible contrasts.\n 2. Efficiency: our palette is diverse enough for our current and future product design, yet values are still predictable and constrained.\n 3. Reusability: our palette is on-brand but versatile. There are very few one-offs that fall outside the palette.\n\n This article shares the process I followed to apply these principles to develop a more adaptable color palette that prioritizes accessibility and is built to scale into all of our future product **design** needs.`)],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getPageContentRefineEvent()],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'What is taylor series?',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.')],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Write a hello world program in c++',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent("Hello! As a helpful and respectful AI assistant, I'd be happy to assist you with your question. However, I'm a text-based AI and cannot provide code in a specific programming language like C++. Instead, I can offer a brief explanation of how to write a \"hello world\" program in C++.\n\nTo write a \"hello world\" program in C++, you can use the following code:\n\n```c++\n#include <iostream>\n\nint main() {\n    std::cout << \"Hello, world!\" << std::endl;\n    return 0;\n}\n```\nThis code will print \"Hello, world!\" and uses `iostream` std library. If you have any further questions or need more information, please don't hesitate to ask!")],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Summarize this excerpt',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SUMMARIZE_SELECTED_TEXT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.',
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getCompletionEvent('Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.')],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Shorten this selected text',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SHORTEN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: 'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getSearchStatusEvent(), getSearchEvent(['pointer compression', 'c++ language specification']), getCompletionEvent('Pointer compression is a memory optimization technique.')],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: 'Will an LTT store backpack fit in a Tesla Model Y frunk?',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SHORTEN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: '',
    edits: [{
      uuid: undefined,
      text: 'Will it fit in a Tesla Model Y frunk?',
      characterType: mojom.CharacterType.HUMAN,
      actionType: mojom.ActionType.SHORTEN,
      visibility: mojom.ConversationTurnVisibility.VISIBLE,
      selectedText: '',
      createdTime: { internalValue: BigInt('13278618001000000') },
      edits: [],
      events: [],
      fromBraveSearchSERP: false
    }],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    fromBraveSearchSERP: false
  },
  {
    uuid: undefined,
    text: '',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [getSearchStatusEvent(), getSearchEvent(['LTT store backpack dimensions', 'Tesla Model Y frunk dimensions'])],
    fromBraveSearchSERP: false
  }
]

const MODELS: mojom.Model[] = [
  {
    key: '1',
    displayName: 'Model One',
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        engineType: mojom.ModelEngineType.LLAMA_REMOTE,
        category: mojom.ModelCategory.CHAT,
        access: mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '2',
    displayName: 'Model Two',
    options: {
      leoModelOptions: {
        name: 'model-two-premium',
        displayMaker: 'Company',
        engineType: mojom.ModelEngineType.LLAMA_REMOTE,
        category: mojom.ModelCategory.CHAT,
        access: mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '3',
    displayName: 'Model Three',
    options: {
      leoModelOptions: {
        name: 'model-three-freemium',
        displayMaker: 'Company',
        engineType: mojom.ModelEngineType.LLAMA_REMOTE,
        category: mojom.ModelCategory.CHAT,
        access: mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700
      },
      customModelOptions: undefined,
    }
  },
  {
    key: '4',
    displayName: 'Microsoft Phi-3',
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

const SITE_INFO: mojom.SiteInfo = {
  uuid: undefined,
  contentType: mojom.ContentType.PageContent,
  title: 'Tiny Tweaks to Neurons Can Rewire Animal Motion',
  contentUsedPercentage: 40,
  isContentAssociationPossible: true,
  hostname: 'www.example.com',
  url: { url: 'https://www.example.com/a' },
  isContentRefined: false,
}

type CustomArgs = {
  currentErrorState: keyof typeof mojom.APIError
  model: string
  inputText: string
  hasConversation: boolean
  hasSuggestedQuestions: boolean
  hasSiteInfo: boolean
  canShowPremiumPrompt: boolean
  hasAcceptedAgreement: boolean
  isPremiumModel: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  suggestionStatus: keyof typeof mojom.SuggestionGenerationStatus
  isMobile: boolean
  isHistoryEnabled: boolean
  isStandalone: boolean
  isDefaultConversation: boolean
  shouldShowLongConversationInfo: boolean
  shouldShowLongPageWarning: boolean
}

const args: CustomArgs = {
  inputText: `Write a Star Trek poem about Data's life on board the Enterprise`,
  hasConversation: true,
  hasSuggestedQuestions: true,
  hasSiteInfo: true,
  canShowPremiumPrompt: false,
  hasAcceptedAgreement: true,
  isPremiumModel: false,
  isPremiumUser: true,
  isPremiumUserDisconnected: false,
  currentErrorState: 'ConnectionIssue' satisfies keyof typeof mojom.APIError,
  suggestionStatus: 'None' satisfies keyof typeof mojom.SuggestionGenerationStatus,
  model: MODELS[0].key,
  isMobile: false,
  isHistoryEnabled: true,
  isStandalone: false,
  isDefaultConversation: true,
  shouldShowLongConversationInfo: false,
  shouldShowLongPageWarning: false,
}

const preview: Meta<CustomArgs> = {
  title: 'Chat/Chat',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    ...InferControlsFromArgs(args),
    currentErrorState: {
      options: getKeysForMojomEnum(mojom.APIError),
      control: { type: 'select' }
    },
    suggestionStatus: {
      options: getKeysForMojomEnum(mojom.SuggestionGenerationStatus),
      control: { type: 'select' }
    },
    model: {
      options: MODELS.map(model => model.displayName),
      control: { type: 'select' }
    },
  },
  args,
  decorators: [
    (Story, options) => {
      const [, setArgs] = useArgs()

      const siteInfo = options.args.hasSiteInfo ? SITE_INFO : new mojom.SiteInfo()
      const suggestedQuestions = options.args.hasSuggestedQuestions
        ? SAMPLE_QUESTIONS
        : siteInfo
          ? [SAMPLE_QUESTIONS[0]]
          : []

      const currentError = mojom.APIError[options.args.currentErrorState]
      const apiHasError = currentError !== mojom.APIError.None
      const currentModel = MODELS.find(m => m.displayName === options.args.model)

      const switchToBasicModel = () => {
        const nonPremiumModel = MODELS.find(model => model.options.leoModelOptions?.access === mojom.ModelAccess.BASIC)
        setArgs({ model: nonPremiumModel })
      }

      const setInputText = (inputText: string) => {
        setArgs({ inputText })
      }

      const aiChatContext: AIChatContext = {
        isDefaultConversation: options.args.isDefaultConversation,
        editingConversationId: null,
        visibleConversations: CONVERSATIONS,
        hasAcceptedAgreement: options.args.hasAcceptedAgreement,
        isPremiumStatusFetching: false,
        isPremiumUser: options.args.isPremiumUser,
        isPremiumUserDisconnected: options.args.isPremiumUserDisconnected,
        canShowPremiumPrompt: options.args.canShowPremiumPrompt,
        isMobile: options.args.isMobile,
        isHistoryEnabled: options.args.isHistoryEnabled,
        isStandalone: options.args.isStandalone,
        allActions: ACTIONS_LIST,
        goPremium: () => {},
        managePremium: () => {},
        handleAgreeClick: () => {},
        dismissPremiumPrompt: () => {},
        userRefreshPremiumSession: () => {},
        onNewConversation: () => {},
        onSelectConversationUuid(id) {},
        setEditingConversationId: () => {}
      }

      const inputText = options.args.inputText

      const conversationContext: ConversationContext = {
        conversationUuid: CONVERSATIONS[1].uuid,
        conversationHistory: options.args.hasConversation ? HISTORY : [],
        associatedContentInfo: siteInfo,
        allModels: MODELS,
        currentModel,
        suggestedQuestions,
        isGenerating: true,
        suggestionStatus: mojom.SuggestionGenerationStatus[options.args.suggestionStatus],
        currentError,
        apiHasError,
        shouldDisableUserInput: false,
        shouldShowLongPageWarning: options.args.shouldShowLongPageWarning,
        shouldShowLongConversationInfo: options.args.shouldShowLongConversationInfo,
        shouldSendPageContents: siteInfo?.isContentAssociationPossible,
        inputText,
        actionList: ACTIONS_LIST,
        selectedActionType: undefined,
        isToolsMenuOpen: false,
        isCurrentModelLeo: true,
        isCharLimitApproaching: inputText.length > 64,
        isCharLimitExceeded: inputText.length > 70,
        inputTextCharCountDisplay: `${inputText.length} / 70`,
        setInputText,
        setCurrentModel: () => {},
        switchToBasicModel,
        generateSuggestedQuestions: () => {},
        dismissLongConversationInfo: () => {},
        updateShouldSendPageContents: () => {},
        retryAPIRequest: () => {},
        handleResetError: () => {},
        submitInputTextToAPI: () => {},
        resetSelectedActionType: () => {},
        handleActionTypeClick: () => {},
        setIsToolsMenuOpen: () => {}
      }

      return (
        <AIChatReactContext.Provider value={aiChatContext}>
          <ConversationReactContext.Provider value={conversationContext}>
          <ThemeProvider>
            <Story />
          </ThemeProvider>
          </ConversationReactContext.Provider>
        </AIChatReactContext.Provider>
      )
    }
  ]
}

export default preview

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
    isStandalone: true
  },
  render: () => {
    return (
      <div className={styles.containerFull}>
        <FullPage />
      </div>
    )
  }
}
