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
import AIChatDataContext, {
  AIChatContext,
  defaultContext
} from '../state/context'
import { useArgs } from '@storybook/addons'

const HISTORY: mojom.ConversationTurn[] = [
  {
    text: 'hello',
    characterType: mojom.CharacterType.HUMAN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    actionType: mojom.ActionType.UNSPECIFIED,
    selectedText: undefined
  },
  {
    text: 'Hi, How are you?',
    characterType: mojom.CharacterType.ASSISTANT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    actionType: mojom.ActionType.UNSPECIFIED,
    selectedText: undefined
  },
  {
    text: 'What is pointer compression?',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'What is taylor series?',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'Write a hello world program in c++',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.QUERY,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: "Hello! As a helpful and respectful AI assistant, I'd be happy to assist you with your question. However, I'm a text-based AI and cannot provide code in a specific programming language like C++. Instead, I can offer a brief explanation of how to write a \"hello world\" program in C++.\n\nTo write a \"hello world\" program in C++, you can use the following code:\n\n```c++\n#include <iostream>\n\nint main() {\n    std::cout << \"Hello, world!\" << std::endl;\n    return 0;\n}\n```\nThis code will print \"Hello, world!\" and uses `iostream` std library. If you have any further questions or need more information, please don't hesitate to ask!",
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'Summarize this excerpt',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SUMMARIZE_SELECTED_TEXT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.'
  },
  {
    text: 'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  },
  {
    text: 'Shorten this selected text',
    characterType: mojom.CharacterType.HUMAN,
    actionType: mojom.ActionType.SHORTEN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: 'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.'
  },
  {
    text: 'Pointer compression is a memory optimization technique.',
    characterType: mojom.CharacterType.ASSISTANT,
    actionType: mojom.ActionType.UNSPECIFIED,
    visibility: mojom.ConversationTurnVisibility.VISIBLE,
    selectedText: undefined
  }
]

const MODELS: mojom.Model[] = [
  {
    key: '1',
    name: 'model-one',
    displayName: 'Model One',
    displayMaker: 'Company',
    engineType: mojom.ModelEngineType.LLAMA_REMOTE,
    category: mojom.ModelCategory.CHAT,
    access: mojom.ModelAccess.BASIC,
    maxPageContentLength: 10000,
    longConversationWarningCharacterLimit: 9700
  },
  {
    key: '2',
    name: 'model-two-premium',
    displayName: 'Model Two',
    displayMaker: 'Company',
    engineType: mojom.ModelEngineType.LLAMA_REMOTE,
    category: mojom.ModelCategory.CHAT,
    access: mojom.ModelAccess.PREMIUM,
    maxPageContentLength: 10000,
    longConversationWarningCharacterLimit: 9700
  },
  {
    key: '3',
    name: 'model-three-freemium',
    displayName: 'Model Three',
    displayMaker: 'Company',
    engineType: mojom.ModelEngineType.LLAMA_REMOTE,
    category: mojom.ModelCategory.CHAT,
    access: mojom.ModelAccess.BASIC_AND_PREMIUM,
    maxPageContentLength: 10000,
    longConversationWarningCharacterLimit: 9700
  }
]

const SAMPLE_QUESTIONS = [
  'Summarize this article',
  'What was the score?',
  'Any injuries?',
  'Why did google executives disregard this character in the company?'
]

const SITE_INFO: mojom.SiteInfo = {
  title: 'Microsoft is hiking the price of Xbox Series X and Xbox Game Pass',
  isContentTruncated: false,
  isContentAssociationPossible: true,
  hostname: 'www.example.com',
  truncatedContentPercentage: 40.5,
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
      options: MODELS.map(m => m.name),
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
    model: MODELS[0].name,
    showAgreementModal: false,
    isMobile: false,
  },
  decorators: [
    (Story: any, options: any) => {
      const [, setArgs] = useArgs()
      const [isGenerating] = React.useState(false)
      const [favIconUrl] = React.useState<string>()
      const hasAcceptedAgreement = options.args.hasAcceptedAgreement

      const siteInfo = options.args.hasSiteInfo ? SITE_INFO : new mojom.SiteInfo()
      const suggestedQuestions = options.args.hasSuggestedQuestions
        ? SAMPLE_QUESTIONS
        : siteInfo
        ? [SAMPLE_QUESTIONS[0]]
        : []

      const currentError = mojom.APIError[options.args.currentErrorState]
      const apiHasError = currentError !== mojom.APIError.None
      const shouldDisableUserInput = apiHasError || isGenerating
      const currentModel = MODELS.find(m => m.name === options.args.model)

      const switchToBasicModel = () => {
        const nonPremiumModel = MODELS.find(m => m.access === mojom.ModelAccess.BASIC)
        setArgs({ model: nonPremiumModel })
      }

      const store: AIChatContext = {
        // Don't error when new properties are added
        ...defaultContext,
        allModels: MODELS,
        currentModel,
        conversationHistory: options.args.hasConversation ? HISTORY : [],
        isGenerating,
        isPremiumStatusFetching: false,
        suggestedQuestions,
        suggestionStatus: mojom.SuggestionGenerationStatus[options.args.suggestionStatus],
        canShowPremiumPrompt: options.args.canShowPremiumPrompt,
        siteInfo,
        favIconUrl,
        currentError,
        hasAcceptedAgreement,
        apiHasError,
        shouldDisableUserInput,
        isPremiumUser: options.args.isPremiumUser,
        isPremiumUserDisconnected: options.args.isPremiumUserDisconnected,
        showAgreementModal: options.args.showAgreementModal,
        isMobile: options.args.isMobile,
        switchToBasicModel,
        shouldShowLongPageWarning: true
      }

      return (
        <AIChatDataContext.Provider value={store}>
          <ThemeProvider>
            <Story />
          </ThemeProvider>
        </AIChatDataContext.Provider>
      )
    }
  ]
}

export const _Panel = (props: {}) => {
  return (
    <div className={styles.container}>
      <Main />
    </div>
  )
}
