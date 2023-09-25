/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withKnobs, select } from '@storybook/addon-knobs'
import styles from './style.module.scss'

import './locale'
import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'

import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import Main from '../components/main'
import { CharacterType, ConversationTurnVisibility, APIError, SiteInfo,AutoGenerateQuestionsPref, ConversationTurn } from '../api/page_handler'
import Context from '../state/context'

const HISTORY = [
  {text: 'What is pointer compression?', characterType: CharacterType.HUMAN, visibility: ConversationTurnVisibility.VISIBLE },
  {text: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.', characterType: CharacterType.ASSISTANT, visibility: ConversationTurnVisibility.VISIBLE },
  {text: 'What is taylor series?', characterType: CharacterType.HUMAN, visibility: ConversationTurnVisibility.VISIBLE },
  {text: 'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.', characterType: CharacterType.ASSISTANT, visibility: ConversationTurnVisibility.VISIBLE },
]

const SAMPLE_QUESTIONS = [
  "Summarize this article",
  "What was the score?",
  "Any injuries?",
  "Why did google executives disregard this character in the company?"
]

const SITE_INFO = {
  title: "Microsoft is hiking the price of Xbox Series X and Xbox Game Pass"
}

interface StoryArgs {
  hasQuestions: boolean
  hasSeenAgreement: boolean
  currentErrorState: APIError
}

export default {
  title: 'Chat/Page',
  parameters: {
    layout: 'centered'
  },
  args: {
    hasQuestions: true,
    hasSeenAgreement: true,
    currentErrorState: select('Current Status', APIError, APIError.RateLimitReached)
  },
  decorators: [
    (Story: any, options: any) => {
      const [conversationHistory] = React.useState<ConversationTurn[]>(HISTORY)
      const [suggestedQuestions] = React.useState<string[]>(SAMPLE_QUESTIONS)
      const [isGenerating] = React.useState(false)
      const [canGenerateQuestions] = React.useState(false)
      const [userAutoGeneratePref] = React.useState<AutoGenerateQuestionsPref>()
      const [siteInfo] = React.useState<SiteInfo | null>(SITE_INFO)
      const [favIconUrl] = React.useState<string>()
      const [currentError] = React.useState<APIError>(options.args.currentErrorState)
      const [hasSeenAgreement] = React.useState(options.args.hasSeenAgreement)

      const generateSuggestedQuestions = () => {}
      const setUserAllowsAutoGenerating = () => {}
      const handleAgreeClick = () => {}

      const apiHasError = (currentError !== APIError.None)
      const shouldDisableUserInput = apiHasError || isGenerating

      const store = {
        conversationHistory,
        isGenerating,
        suggestedQuestions,
        canGenerateQuestions,
        userAutoGeneratePref,
        siteInfo,
        favIconUrl,
        currentError,
        hasSeenAgreement,
        apiHasError,
        shouldDisableUserInput,
        generateSuggestedQuestions,
        setUserAllowsAutoGenerating,
        handleAgreeClick
      }

      return (
        <Context.Provider value={store}>
          <ThemeProvider>
            <Story />
          </ThemeProvider>
        </Context.Provider>
      )
    },
    withKnobs
  ]
}

export const _Main = (props: StoryArgs) => {
  return (
    <div className={styles.container}>
      <Main />
    </div>
  )
}
