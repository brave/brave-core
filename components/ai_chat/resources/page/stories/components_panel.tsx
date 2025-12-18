/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../../common/strings'

import * as React from 'react'
import { useArgs } from '@storybook/preview-api'
import { Meta, StoryObj } from '@storybook/react'
import '@brave/leo/tokens/css/variables.css'
import '$web-components/app.global.scss'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { InferControlsFromArgs } from '../../../../../.storybook/utils'
import * as Mojom from '../../common/mojom'
import {
  ActiveChatContext,
  SelectedChatDetails,
} from '../state/active_chat_context'
import { AIChatProvider } from '../state/ai_chat_context'
import { ConversationProvider } from '../state/conversation_context'
import FeedbackForm from '../components/feedback_form'
import FullPage from '../components/full_page'
import Loading from '../components/loading'
import Main from '../components/main'
import ACTIONS_LIST from './story_utils/actions'
import styles from './style.module.scss'
import StorybookConversationEntries from './story_utils/ConversationEntries'
import {
  UntrustedConversationContext,
  UntrustedConversationReactContext,
} from '../../untrusted_conversation_frame/untrusted_conversation_context'
import ErrorConnection from '../components/alerts/error_connection'
import ErrorConversationEnd from '../components/alerts/error_conversation_end'
import ErrorInvalidAPIKey from '../components/alerts/error_invalid_api_key'
import ErrorInvalidEndpointURL from '../components/alerts/error_invalid_endpoint_url'
import ErrorRateLimit from '../components/alerts/error_rate_limit'
import ErrorServiceOverloaded from '../components/alerts/error_service_overloaded'
import LongConversationInfo from '../components/alerts/long_conversation_info'
import WarningPremiumDisconnected from '../components/alerts/warning_premium_disconnected'
import Attachments from '../components/attachments'
import { createTextContentBlock } from '../../common/content_block'
import ToolEvent from '../../untrusted_conversation_frame/components/assistant_response/tool_event'
import { taskConversationEntries } from './story_utils/history'
import { toolUseCompleteAssistantDetailStorage } from './story_utils/events'
import { Content } from '../components/input_box/editable_content'
import { getToolUseEvent } from '../../common/test_data_utils'
import createAIChatApi from '../api'
import createConversationApi from '../api/conversation_api'
import {
  createMockConversationHandler,
  createMockService,
  createMockUIHandler,
  createMockBookmarksService,
  createMockHistoryService,
  createMockMetrics,
} from '../api/mock_interfaces'

// TODO(https://github.com/brave/brave-browser/issues/47810): Attempt to split this file up

const eventTemplate: Mojom.ConversationEntryEvent = {
  completionEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  conversationTitleEvent: undefined,
  sourcesEvent: undefined,
  contentReceiptEvent: undefined,
  toolUseEvent: undefined,
}

function getCompletionEvent(text: string): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    completionEvent: { completion: text },
  }
}

function getSearchEvent(queries: string[]): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    searchQueriesEvent: { searchQueries: queries },
  }
}

function getSearchStatusEvent(): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    searchStatusEvent: { isSearching: true },
  }
}

function getWebSourcesEvent(
  sources: Mojom.WebSource[],
): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    sourcesEvent: { sources, richResults: [] },
  }
}

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

const toolEvents: Mojom.ConversationEntryEvent[] = [
  getToolUseEvent({
    id: 'abc123d',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: [createTextContentBlock('7:00pm')],
  }),
  getToolUseEvent({
    id: 'abc123e',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: undefined,
  }),
  getToolUseEvent({
    id: 'abc123f',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: undefined,
  }),
  getToolUseEvent({
    id: 'abc123g',
    toolName: Mojom.NAVIGATE_TOOL_NAME,
    argumentsJson: JSON.stringify({ website_url: 'https://www.example.com' }),
    output: undefined,
  }),
  getToolUseEvent({
    id: 'abc123h',
    toolName: Mojom.CODE_EXECUTION_TOOL_NAME,
    argumentsJson: JSON.stringify({
      script: 'const result = 1 + 2;\nreturn `1 + 2 = ${result};`',
    }),
    output: undefined,
  }),
  getToolUseEvent({
    id: 'abc123i',
    toolName: Mojom.CODE_EXECUTION_TOOL_NAME,
    argumentsJson: JSON.stringify({
      script: 'const result = 1 + 2;\nreturn `1 + 2 = ${result}`;',
    }),
    output: [createTextContentBlock('1 + 2 = 3')],
  }),
]

const MEMORY_HISTORY: Mojom.ConversationTurn[] = [
  {
    uuid: 'user-1',
    text: 'Remember that I work as a software engineer.',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'assistant-1',
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001100000') },
    events: [
      getToolUseEvent({
        id: 'memory-1',
        toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
        argumentsJson: '{"memory": "works as a software engineer"}',
        output: [createTextContentBlock('')],
      }),
      getCompletionEvent("I'll remember that you work as a software engineer."),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'user-2',
    text: 'Remember I like cats.',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001200000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'assistant-2',
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001300000') },
    events: [
      getToolUseEvent({
        id: 'memory-2',
        toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
        argumentsJson: '{"memory": "Likes cats"}',
        output: [createTextContentBlock('')],
      }),
      getCompletionEvent("I've noted you like cats."),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'user-3',
    text: 'Remember my favorite hobby is hiking.',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001400000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'assistant-3',
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001500000') },
    events: [
      getToolUseEvent({
        id: 'memory-3',
        toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
        argumentsJson: '{"memory": "favorite hobby is hiking"}',
        output: [createTextContentBlock('Memory storage failed')],
      }),
      getCompletionEvent(
        'I encountered an error while trying to store that information.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
]

const HISTORY: Mojom.ConversationTurn[] = [
  {
    uuid: 'skill-turn',
    text: '/translate Hello world, how are you today?',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618000900000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: {
      shortcut: 'translate',
      prompt: 'Translate the following text to English',
    },
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'skill-response',
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618000950000') },
    events: [
      getCompletionEvent(
        'Here is the translation:\n\n"Hello world, how are you today?"'
          + '\n\nThis text is already in English, so no translation is needed.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: 'turn-uuid',
    text: 'Summarize this page',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SUMMARIZE_PAGE,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        'The ways that animals move are just about as myriad as the animal kingdom itself. They walk, run, swim, crawl, fly and slither — and within each of those categories lies a tremendous number of subtly different movement types. A seagull and a *hummingbird* both have wings, but otherwise their flight techniques and abilities are poles apart. Orcas and **piranhas** both have tails, but they accomplish very different types of swimming. Even a human walking or running is moving their body in fundamentally different ways.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: undefined,
    text: 'Question that results in a task',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  // Show how a Task is displayed within history. Use the AssistantTask story for
  // viewing an active Task.
  ...taskConversationEntries,
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
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        `# Title 1\n ## Title 2\n ## **Title 2** using bold that doesn't look different\n### Title 3\n#### Title 4\n \nDuring the latter part of 2021, I reflected on the challenges we were facing at Modern Health. One recurring problem that stood out was our struggle to create new products with an unstructured color palette. This resulted in poor [communication](https://www.google.com) between designers and developers, an inconsistent product brand, and increasing accessibility problems.\n\n1. Inclusivity: our palette provides easy ways to ensure our product uses accessible contrasts.\n 2. Efficiency: our palette is diverse enough for our current and future product design, yet values are still predictable and constrained.\n 3. Reusability: our palette is on-brand but versatile. There are very few one-offs that fall outside the palette.\n\n This article shares the process I followed to apply these principles to develop a more adaptable color palette that prioritizes accessibility and is built to scale into all of our future product **design** needs.`,
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        "Sure! Here's a table with 5 Marvel characters:\n\n"
          + '| First Name | Last Name   | Character Name       | '
          + 'First Appearance |\n'
          + '|------------|-------------|----------------------|'
          + '------------------|\n'
          + '| Tony       | Stark      | Iron Man            | '
          + '1968              |\n'
          + '| Steve      | Rogers     | Captain America      | '
          + '1941              |\n'
          + '| Thor       | Odinson    | Thor                 | '
          + '1962              |\n'
          + '| Natasha    | Romanoff   | Black Widow          | '
          + '1964              |\n'
          + '| Peter      | Parker     | Spider-Man           | '
          + '1962              |\n'
          + "\n\n Let me know if you'd like more details!",
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: undefined,
    text: 'Shorten this selected text',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SHORTEN,
    prompt: undefined,
    selectedText:
      'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        '[1]:https://www.example.com\n[2]:https://lttstore.com\n[3]:https://www.tesla.com/modely\n[Pointer compression](https://www.example.com) is a [memory](https://brave.com/wont-show-as-link) optimization technique.[1][3]',
      ),
      getWebSourcesEvent([
        {
          url: { url: 'https://www.example.com' },
          title: 'Pointer Compression',
          faviconUrl: { url: 'https://www.example.com/favicon.ico' },
        },
        {
          title: 'LTT Store',
          faviconUrl: { url: 'https://lttstore.com/favicon.ico' },
          url: { url: 'https://lttstore.com' },
        },
        {
          title: 'Tesla Model Y',
          faviconUrl: { url: 'https://www.tesla.com/favicon.ico' },
          url: { url: 'https://www.tesla.com/modely' },
        },
      ]),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: undefined,
    text: 'Will an LTT store backpack fit in a Tesla Model Y frunk?',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.SHORTEN,
    prompt: undefined,
    selectedText: '',
    edits: [
      {
        uuid: undefined,
        text: 'Will it fit in a Tesla Model Y frunk?',
        characterType: Mojom.CharacterType.HUMAN,
        actionType: Mojom.ActionType.SHORTEN,
        prompt: undefined,
        selectedText: '',
        createdTime: { internalValue: BigInt('13278618001000000') },
        edits: [],
        events: [],
        uploadedFiles: [],
        fromBraveSearchSERP: false,
        skill: undefined,
        modelKey: '1',
        nearVerificationStatus: undefined,
      },
    ],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getSearchEvent([
        'LTT store backpack dimensions',
        'Tesla Model Y frunk dimensions',
      ]),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
    uploadedFiles: [
      {
        filename: 'lion.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kImage,
      },
    ],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: undefined,
    text: 'Summarize this page',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [
      {
        filename: 'full_screenshot_0.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
      },
      {
        filename: 'full_screenshot_1.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
      },
    ],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        'This website compares differences between Juniper Model Y and legacy one.',
      ),
      toolUseCompleteAssistantDetailStorage,
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  {
    uuid: undefined,
    text: 'Summarize these',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.QUERY,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [
      {
        filename: 'full_screenshot_0.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
      },
      {
        filename: 'full_screenshot_1.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
      },
      {
        filename: 'lion.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kImage,
      },
    ],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        'According to screenshots, this website compares differences between Juniper Model Y and legacy one. And a lion image.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
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
      getCompletionEvent(
        "Sure! Here's a table with 5 Marvel characters:\n\n"
          + '| First Name | Last Name   | Character Name       | '
          + 'First Appearance |\n'
          + '|------------|-------------|----------------------|'
          + '------------------|\n'
          + '| Tony       | Stark      | Iron Man            | '
          + '1968              |\n'
          + '| Steve      | Rogers     | Captain America      | '
          + '1941              |\n'
          + '| Thor       | Odinson    | Thor                 | '
          + '1962              |\n'
          + '| Natasha    | Romanoff   | Black Widow          | '
          + '1964              |\n'
          + '| Peter      | Parker     | Spider-Man           | '
          + '1962              |\n'
          + "\n\n Let me know if you'd like more details!",
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  // Show that tool use events are NOT interactive if they are not the last entry in the
  // group of assistant conversation entries.
  {
    uuid: undefined,
    text: '',
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [...toolEvents.slice(2)],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
  // Show that single or multipletool use events are interactive if they are the
  // last entry in thegroup of assistant conversation entries.
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
      getCompletionEvent('Answer one of these questions:'),
      ...toolEvents.slice(0, 3),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
    nearVerificationStatus: undefined,
  },
]

const MODELS: Mojom.Model[] = [
  {
    key: '1',
    displayName: 'Model One',
    visionSupport: false,
    supportsTools: false,
    isSuggestedModel: true,
    isNearModel: false,
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
    isNearModel: false,
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
    isNearModel: false,
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

type CustomArgs = {
  initialized: boolean
  currentErrorState: keyof typeof Mojom.APIError
  toolUseTaskState: keyof typeof Mojom.TaskState
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
  isToolExecuting: boolean
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
  useMemoryHistory: false,
  skillDialog: null,
  selectedActionType: undefined,
  selectedSkill: undefined,
}

const meta: Meta<CustomArgs> = {
  title: 'Chat/Chat',
  parameters: {
    layout: 'centered',
  },
  argTypes: {
    ...InferControlsFromArgs(args),
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
      const [, setArgs] = useArgs()
      return (
        <StoryContext
          args={options.args}
          setArgs={setArgs}
        >
          <Story />
        </StoryContext>
      )
    },
  ],
}

// Sample data for Storybook
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

/**
 * Creates the AIChatApi using real factory with mock handlers.
 * Returns a stable reference that won't change across renders.
 *
 * Uses a ref pattern so mock functions always read current storybook args.
 * Invalidate queries when relevant storybook controls change.
 *
 * @param args - CustomArgs from storybook controls (used via ref for reactivity)
 */
function useStoryAIChatApi(args: CustomArgs) {
  // Ref holds current args - updated every render before any queries
  const argsRef = React.useRef(args)
  argsRef.current = args

  const [aiChatApi] = React.useState(() => {
    // Compute conversations list based on args
    const getConversationsForArgs = () => {
      const count = argsRef.current.conversationListCount
      if (CONVERSATIONS.length <= count) {
        return CONVERSATIONS
      }
      return CONVERSATIONS.slice(0, count)
    }

    const mockService = createMockService({
      // Mocks read from ref, always getting current storybook args
      getConversations: () =>
        Promise.resolve({ conversations: getConversationsForArgs() }),
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
    })

    const mockUIHandler = createMockUIHandler()

    const mockBookmarksService = createMockBookmarksService({
      getBookmarks: () => Promise.resolve({ bookmarks: SAMPLE_BOOKMARKS }),
    })

    const mockHistoryService = createMockHistoryService({
      getHistory: () =>
        Promise.resolve({
          history: argsRef.current.isHistoryEnabled
            ? SAMPLE_HISTORY_ENTRIES
            : [],
        }),
    })

    const mockMetrics = createMockMetrics()

    const api = createAIChatApi(
      mockService,
      mockUIHandler,
      mockBookmarksService,
      mockHistoryService,
      mockMetrics,
    )

    // Push initial state data synchronously (state endpoints don't have query
    // functions so they need to be updated directly)
    api.api.tabs.update(SAMPLE_TABS)
    api.api.isStandalone.update(argsRef.current.isStandalone)
    api.api.state.update({
      hasAcceptedAgreement: argsRef.current.hasAcceptedAgreement,
      isStoragePrefEnabled: argsRef.current.isStoragePrefEnabled,
      isStorageNoticeDismissed: argsRef.current.isStorageNoticeDismissed,
      canShowPremiumPrompt: argsRef.current.canShowPremiumPrompt,
    })

    return api
  })

  // When storybook controls change, invalidate all queries to trigger refetch.
  // The ref already has new data, invalidation causes queries to re-run with
  // fresh data from the mock functions.
  React.useEffect(() => {
    aiChatApi.api.invalidateAll()
  }, [aiChatApi.api, args])

  // Update state endpoints when their args change (these don't have query
  // functions so invalidateAll won't help - they need direct updates)
  React.useLayoutEffect(() => {
    aiChatApi.api.state.update({
      hasAcceptedAgreement: args.hasAcceptedAgreement,
      isStoragePrefEnabled: args.isStoragePrefEnabled,
      isStorageNoticeDismissed: args.isStorageNoticeDismissed,
      canShowPremiumPrompt: args.canShowPremiumPrompt,
    })
    aiChatApi.api.isStandalone.update(args.isStandalone)
  }, [
    aiChatApi.api,
    args.hasAcceptedAgreement,
    args.isStoragePrefEnabled,
    args.isStorageNoticeDismissed,
    args.canShowPremiumPrompt,
    args.isStandalone,
  ])

  return aiChatApi
}

/**
 * Creates the ConversationApi using real factory with mock handlers.
 * Returns a stable reference that won't change across renders.
 *
 * Uses a ref pattern so mock functions always read current storybook args.
 * Invalidate queries when relevant storybook controls change.
 *
 * @param args - CustomArgs from storybook controls (used via ref for reactivity)
 */
function useStoryConversationApi(args: CustomArgs) {
  // Ref holds current args - updated every render before any queries
  const argsRef = React.useRef(args)
  argsRef.current = args

  const [conversationApi] = React.useState(() => {
    // Helper to compute derived values from current args
    const getAssociatedContent = () =>
      argsRef.current.hasAssociatedContent
        ? ASSOCIATED_CONTENT
        : new Mojom.AssociatedContent()

    const getSuggestedQuestions = () =>
      argsRef.current.hasSuggestedQuestions
        ? SAMPLE_QUESTIONS
        : argsRef.current.hasAssociatedContent
          ? [SAMPLE_QUESTIONS[0]]
          : []

    const getCurrentModel = () =>
      MODELS.find((m) => m.displayName === argsRef.current.model)

    const getConversationHistory = () =>
      argsRef.current.hasConversation
        ? argsRef.current.useMemoryHistory
          ? MEMORY_HISTORY
          : HISTORY
        : []

    const mockHandler = createMockConversationHandler({
      // Mocks read from ref, always getting current storybook args
      getState: () =>
        Promise.resolve({
          conversationState: {
            conversationUuid: argsRef.current.isNewConversation
              ? 'new-conversation'
              : CONVERSATIONS[1].uuid,
            isRequestInProgress: argsRef.current.isGenerating,
            currentModelKey: getCurrentModel()?.key ?? MODELS[0].key,
            defaultModelKey: MODELS[0].key,
            allModels: MODELS,
            suggestedQuestions: getSuggestedQuestions(),
            suggestionStatus:
              Mojom.SuggestionGenerationStatus[
                argsRef.current.suggestionStatus
              ],
            associatedContent: [getAssociatedContent()],
            error: Mojom.APIError[argsRef.current.currentErrorState],
            temporary: argsRef.current.isTemporaryChat,
            toolUseTaskState: Mojom.TaskState[argsRef.current.toolUseTaskState],
          },
        }),
      getConversationHistory: () =>
        Promise.resolve({ conversationHistory: getConversationHistory() }),
    })

    const api = createConversationApi(mockHandler)

    return api
  })

  // When storybook controls change, invalidate all queries to trigger refetch.
  // The ref already has new data, invalidation causes queries to re-run with
  // fresh data from the mock functions.
  React.useEffect(() => {
    conversationApi.api.invalidateAll()
  }, [conversationApi.api, args])

  return conversationApi
}

function StoryContext(
  props: React.PropsWithChildren<{
    args: CustomArgs
    setArgs: (newArgs: Partial<CustomArgs>) => void
  }>,
) {
  const { args: storyArgs, setArgs: _setArgs } = props
  // setArgs is available for future use when we need to update storybook
  // controls from within the story (e.g. for interactive actions)
  void _setArgs

  // Create APIs using real factories with mock handlers.
  // The hooks use a ref pattern so mock functions always read current args.
  // Query invalidation happens inside the hooks when relevant args change.
  const aiChatApi = useStoryAIChatApi(storyArgs)
  const conversationApi = useStoryConversationApi(storyArgs)

  // Compute derived values from args for UntrustedConversationContext
  const associatedContent = storyArgs.hasAssociatedContent
    ? ASSOCIATED_CONTENT
    : new Mojom.AssociatedContent()

  const currentError = Mojom.APIError[storyArgs.currentErrorState]
  const currentModel = MODELS.find((m) => m.displayName === storyArgs.model)

  const conversationHistory = storyArgs.hasConversation
    ? storyArgs.useMemoryHistory
      ? MEMORY_HISTORY
      : HISTORY
    : []

  // ActiveChatContext for conversation selection
  const activeChatContext: SelectedChatDetails = React.useMemo(
    () => ({
      api: conversationApi.api,
      selectedConversationId: CONVERSATIONS[0].uuid,
      updateSelectedConversationId: () => {},
      createNewConversation: () => {},
      isTabAssociated: storyArgs.isDefaultConversation,
    }),
    [conversationApi.api, storyArgs.isDefaultConversation],
  )

  // UntrustedConversationContext - still uses manual object since not migrated
  const conversationEntriesContext: UntrustedConversationContext =
    React.useMemo(
      () => ({
        conversationHistory,
        conversationCapability: Mojom.ConversationCapability.CONTENT_AGENT,
        isGenerating: storyArgs.isGenerating,
        isToolExecuting: storyArgs.isToolExecuting,
        toolUseTaskState: Mojom.TaskState[storyArgs.toolUseTaskState],
        isLeoModel: true,
        contentUsedPercentage: storyArgs.shouldShowLongPageWarning ? 48 : 100,
        visualContentUsedPercentage:
          storyArgs.shouldShowLongVisualContentWarning ? 75 : undefined,
        totalTokens: BigInt(storyArgs.totalTokens),
        trimmedTokens: BigInt(storyArgs.trimmedTokens),
        canSubmitUserEntries: currentError === Mojom.APIError.None,
        isMobile: storyArgs.isMobile,
        allModels: MODELS,
        currentModelKey: currentModel?.key ?? '',
        associatedContent: [associatedContent],
        isPremiumUser: storyArgs.isPremiumUser,
        uiHandler: {
          hasMemory: (memory: string) => {
            // Return false for the "undone" example to show undone state
            if (memory === 'Likes cats') {
              return Promise.resolve({ exists: false })
            }
            // Return true for others to show success state
            return Promise.resolve({ exists: true })
          },
        } as unknown as Mojom.UntrustedUIHandlerRemote,
      }),
      [
        conversationHistory,
        storyArgs.isGenerating,
        storyArgs.isToolExecuting,
        storyArgs.toolUseTaskState,
        storyArgs.shouldShowLongPageWarning,
        storyArgs.shouldShowLongVisualContentWarning,
        storyArgs.totalTokens,
        storyArgs.trimmedTokens,
        storyArgs.isMobile,
        storyArgs.isPremiumUser,
        currentError,
        currentModel?.key,
        associatedContent,
      ],
    )

  return (
    <AIChatProvider
      api={aiChatApi.api}
      conversationEntriesComponent={StorybookConversationEntries}
      // Overrides for values that come from internal hooks (useState, etc.)
      // and can't be controlled via API mocks
      overrides={{
        editingConversationId: storyArgs.editingConversationId,
        deletingConversationId: storyArgs.deletingConversationId,
        initialized: storyArgs.initialized,
        isAIChatAgentProfileFeatureEnabled:
          storyArgs.isAIChatAgentProfileFeatureEnabled,
        isAIChatAgentProfile: storyArgs.isAIChatAgentProfile,
        isMobile: storyArgs.isMobile,
        isHistoryFeatureEnabled: storyArgs.isHistoryEnabled,
        skillDialog: storyArgs.skillDialog,
      }}
    >
      <ActiveChatContext.Provider value={activeChatContext}>
        <ConversationProvider
          api={conversationApi.api}
          selectedConversationId={CONVERSATIONS[0].uuid}
          updateSelectedConversationId={() => {}}
          createNewConversation={() => {}}
          isTabAssociated={storyArgs.isDefaultConversation}
          // Overrides for values from internal hooks (useSendFeedback, useState)
          overrides={{
            isFeedbackFormVisible: storyArgs.isFeedbackFormVisible,
            ratingTurnUuid: storyArgs.ratingTurnUuid,
            inputText: storyArgs.inputText,
            selectedActionType: storyArgs.selectedActionType,
            selectedSkill: storyArgs.selectedSkill,
            attachmentsDialog: storyArgs.attachmentsDialog,
            isDragActive: storyArgs.isDragActive,
            isDragOver: storyArgs.isDragOver,
            generatedUrlToBeOpened: storyArgs.generatedUrlToBeOpened,
          }}
        >
          <UntrustedConversationReactContext.Provider
            value={conversationEntriesContext}
          >
            {props.children}
          </UntrustedConversationReactContext.Provider>
        </ConversationProvider>
      </ActiveChatContext.Provider>
    </AIChatProvider>
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
  },
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
  },
}

export const _FeedbackForm = {
  render: () => {
    return (
      <div className={styles.container}>
        <FeedbackForm />
      </div>
    )
  },
}

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
    hasConversation: false,
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
export const _AttachmentsPanel = {
  args: {
    isStandalone: true,
    isDefaultConversation: false,
  },
  render: () => {
    return (
      <div className={styles.container}>
        <Attachments />
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

export const _ToolUse = {
  render: () => {
    return (
      <div className={styles.container}>
        {toolEvents
          .filter(
            (event) =>
              event.toolUseEvent!.toolName !== Mojom.MEMORY_STORAGE_TOOL_NAME,
          )
          .map((event) => (
            <ToolEvent
              key={event.toolUseEvent!.id}
              toolUseEvent={event.toolUseEvent!}
              isExecuting={args.isToolExecuting}
              isEntryActive
            ></ToolEvent>
          ))}
      </div>
    )
  },
}

export const _MemoryEvents: Story = {
  render: (args) => {
    return (
      <div className={styles.container}>
        <Main />
      </div>
    )
  },
  args: {
    ...args,
    hasConversation: true,
    useMemoryHistory: true,
  },
}
