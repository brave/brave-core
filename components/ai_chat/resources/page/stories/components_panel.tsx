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

const HISTORY = [
  {
    text: 'What is pointer compression?',
    characterType: mojom.CharacterType.HUMAN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE
  },
  {
    text: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.',
    characterType: mojom.CharacterType.ASSISTANT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE
  },
  {
    text: 'What is taylor series?',
    characterType: mojom.CharacterType.HUMAN,
    visibility: mojom.ConversationTurnVisibility.VISIBLE
  },
  {
    text: 'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.',
    characterType: mojom.CharacterType.ASSISTANT,
    visibility: mojom.ConversationTurnVisibility.VISIBLE
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
    isPremium: false,
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
    isPremium: true,
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

const SITE_INFO = {
  title: 'Microsoft is hiking the price of Xbox Series X and Xbox Game Pass',
  isContentTruncated: false
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
    suggestedQuestionsPref: {
      options: getKeysForMojomEnum(mojom.AutoGenerateQuestionsPref),
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
    showModelIntro: true,
    canShowPremiumPrompt: false,
    hasAcceptedAgreement: true,
    isPremiumModel: false,
    isPremiumUser: true,
    isPremiumUserDisconnected: false,
    currentErrorState: 'ConnectionIssue' satisfies keyof typeof mojom.APIError,
    suggestedQuestionsPref: 'Unset' satisfies keyof typeof mojom.AutoGenerateQuestionsPref,
    model: MODELS[0].name
  },
  decorators: [
    (Story: any, options: any) => {
      const [isGenerating] = React.useState(false)
      const [canGenerateQuestions] = React.useState(false)
      const userAutoGeneratePref: mojom.AutoGenerateQuestionsPref = mojom.AutoGenerateQuestionsPref[options.args.suggestedQuestionsPref]
      const [favIconUrl] = React.useState<string>()
      const hasAcceptedAgreement = options.args.hasAcceptedAgreement

      const siteInfo = options.args.hasSiteInfo ? SITE_INFO : null
      const suggestedQuestions = options.args.hasSuggestedQuestions
        ? SAMPLE_QUESTIONS
        : siteInfo
        ? [SAMPLE_QUESTIONS[0]]
        : []

      const currentError = mojom.APIError[options.args.currentErrorState]
      const apiHasError = currentError !== mojom.APIError.None
      const shouldDisableUserInput = apiHasError || isGenerating

      const store: AIChatContext = {
        // Don't error when new properties are added
        ...defaultContext,
        showModelIntro: options.args.showModelIntro,
        allModels: MODELS,
        currentModel: MODELS.find(m => m.name === options.args.model),
        conversationHistory: options.args.hasConversation ? HISTORY : [],
        isGenerating,
        isPremiumStatusFetching: false,
        suggestedQuestions,
        canGenerateQuestions,
        userAutoGeneratePref,
        canShowPremiumPrompt: options.args.canShowPremiumPrompt,
        siteInfo,
        favIconUrl,
        currentError,
        hasAcceptedAgreement,
        apiHasError,
        shouldDisableUserInput,
        isPremiumUser: options.args.isPremiumUser,
        isPremiumUserDisconnected: options.args.isPremiumUserDisconnected
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
