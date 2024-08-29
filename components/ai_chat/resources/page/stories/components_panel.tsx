/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'

import './locale'
import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'
import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import Main from '../components/main'
import * as mojom from '../api/page_handler'
import { useArgs } from '@storybook/preview-api'
import FeedbackForm from '../components/feedback_form'
import DataContextProvider from '../state/data-context-provider'
import { setPageHandlerAPIForTesting } from '../api/page_handler'
import { MockPageHandlerAPI } from '../api/mock_page_handler'

const mockAPIHandler = new MockPageHandlerAPI()
setPageHandlerAPIForTesting(mockAPIHandler as any)

function getCompletionEvent(text: string): mojom.ConversationEntryEvent {
  return {
    completionEvent: { completion: text },
    searchQueriesEvent: undefined,
    searchStatusEvent: undefined
  }
}

function getSearchEvent(queries: string[]): mojom.ConversationEntryEvent {
  return {
    completionEvent: undefined,
    searchQueriesEvent: { searchQueries: queries },
    searchStatusEvent: undefined
  }
}

function getSearchStatusEvent(): mojom.ConversationEntryEvent {
  return {
    completionEvent: undefined,
    searchQueriesEvent: undefined,
    searchStatusEvent: { isSearching: true }
  }
}

const HISTORY: mojom.ConversationTurn[] = [
  {
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
    text: 'What is pointer compression?',
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
    text: 'Will an LTT store backpack fit in a Tesla Model Y frunk?',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SHORTEN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: '',
    edits: [{
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
        maxPageContentLength: 10000,
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
        maxPageContentLength: 10000,
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
        maxPageContentLength: 10000,
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
  title: 'Tiny Tweaks to Neurons Can Rewire Animal Motion',
  contentUsedPercentage: 40,
  isContentAssociationPossible: true,
  hostname: 'www.example.com',
  isContentRefined: false,
}

export default {
  title: 'Chat/Chat',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
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
    }
  },
  args: {
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
    showAgreementModal: false,
    isMobile: false,
    shouldShowLongConversationInfo: false,
    shouldShowLongPageWarning: false,
  },
  decorators: [
    (Story: any, options: any) => {
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

      const store = {
        allModels: MODELS,
        conversationHistory: options.args.hasConversation ? HISTORY : [],
        isPremiumStatusFetching: false,
        isGenerating: true,
        suggestionStatus: mojom.SuggestionGenerationStatus[options.args.suggestionStatus],
        canShowPremiumPrompt: options.args.canShowPremiumPrompt,
        isPremiumUser: options.args.isPremiumUser,
        isPremiumUserDisconnected: options.args.isPremiumUserDisconnected,
        showAgreementModal: options.args.showAgreementModal,
        isMobile: options.args.isMobile,
        shouldShowLongPageWarning: options.args.shouldShowLongPageWarning,
        shouldShowLongConversationInfo: options.args.shouldShowLongConversationInfo,
        currentModel,
        suggestedQuestions,
        siteInfo,
        currentError,
        hasAcceptedAgreement: options.args.hasAcceptedAgreement,
        apiHasError,
        switchToBasicModel,
        shouldSendPageContents: siteInfo?.isContentAssociationPossible
      }

      return (
        <DataContextProvider store={store}>
          <ThemeProvider>
            <Story />
          </ThemeProvider>
        </DataContextProvider>
      )
    }
  ]
}

export const _Panel = {
  render: () => {
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
