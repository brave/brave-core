/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withKnobs } from '@storybook/addon-knobs'
import styles from './style.module.scss'

import './locale'
import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'

import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import Main from '../components/main'
import ConversationList from '../components/conversation_list'
import InputBox from '../components/input_box'
import { useInput } from '../state/hooks'
import { CharacterType, ConversationTurnVisibility, ConversationTurnStatus}
  from '../api/page_handler'
import PrivacyMessage from '../components/privacy_message'
import SiteTitle from '../components/site_title'
import PromptAutoSuggestion from '../components/prompt_auto_suggestion'

const DATA = [
  {text: 'What is pointer compression?', characterType: CharacterType.HUMAN, visibility: ConversationTurnVisibility.VISIBLE, status: ConversationTurnStatus.NORMAL, uuid: '2cf1535b-151c-4301-9f0c-901f1ec29803' },
  {text: 'Pointer compression is a memory optimization technique where pointers (memory addresses) are stored in a compressed format to save memory. The basic idea is that since most pointers will be clustered together and point to objects allocated around the same time, you can store a compressed representation of the pointer and decompress it when needed. Some common ways this is done: Store an offset from a base pointer instead of the full pointer value Store increments/decrements from the previous pointer instead of the full value Use pointer tagging to store extra information in the low bits of the pointer Encode groups of pointers together The tradeoff is some extra CPU cost to decompress the pointers, versus saving memory. This technique is most useful in memory constrained environments.', characterType: CharacterType.ASSISTANT, visibility: ConversationTurnVisibility.VISIBLE, status: ConversationTurnStatus.NORMAL, uuid: '2607a8b9-b0c9-4adf-89f8-00d0e6ed130c' },
  {text: 'What is taylor series?', characterType: CharacterType.HUMAN, visibility: ConversationTurnVisibility.VISIBLE, status: ConversationTurnStatus.NORMAL, uuid: '1165c6c9-2720-4d51-b635-c22bc9fb7d59' },
  {text: 'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.', characterType: CharacterType.ASSISTANT, visibility: ConversationTurnVisibility.VISIBLE, status: ConversationTurnStatus.NORMAL, uuid: 'fd425eef-09b5-4534-ae61-3c8dc2a94160' },
]

const SAMPLE_QUESTIONS = [
  "Summarize this article",
  "What was the score?",
  "Any injuries?",
  "Why did google executives disregard this character in the company?"
]

interface StoryProps {
  hasQuestions: boolean
}

export default {
  title: 'Chat/Page',
  parameters: {
    layout: 'centered'
  },
  args: {
    hasQuestions: true
  },
  decorators: [
    (Story: any) => {
      return (
        <ThemeProvider>
          <Story />
        </ThemeProvider>
      )
    },
    withKnobs
  ]
}

export const _Main = (props: StoryProps) => {
  const { value, setValue } = useInput();
  const [hasSeenAgreement] = React.useState(true)

  const handleSubmit = () => {
    setValue('')
  }

  const handleInputChange = (e: any) => {
    const target = e.target as HTMLInputElement
    setValue(target.value)
  }

  let conversationList = <PrivacyMessage />
  let siteTitleElement = null
  let promptAutoSuggestionElement = null

  if (hasSeenAgreement) {
    conversationList = (
      <ConversationList
        list={DATA}
        isLoading={false}
        suggestedQuestions={props.hasQuestions ? SAMPLE_QUESTIONS : []}
        onQuestionSubmit={() => {}}
        onRetrySubmit={() => {}}
      />
    )

    siteTitleElement = (
      <SiteTitle siteInfo={{ title: "Microsoft is hiking the price of Xbox Series X and Xbox Game Pass" }} favIconUrl="" />
    )

    promptAutoSuggestionElement = (
      <PromptAutoSuggestion
      />
    )
  }

  const inputBox = (
    <InputBox
      value={value}
      onInputChange={handleInputChange}
      onSubmit={handleSubmit}
      hasSeenAgreement={hasSeenAgreement}
      onHandleAgreeClick={() => {}}
    />
  )

  return (
    <div className={styles.container}>
      <Main
        conversationList={conversationList}
        inputBox={inputBox}
        siteTitle={siteTitleElement}
        promptAutoSuggestion={promptAutoSuggestionElement}
      />
    </div>
  )
}
