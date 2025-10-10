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
import {
  AIChatContext,
  AIChatReactContext,
  useIsSmall,
} from '../state/ai_chat_context'
import {
  ConversationContext,
  ConversationReactContext,
} from '../state/conversation_context'
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

// TODO(https://github.com/brave/brave-browser/issues/47810): Attempt to split this file up

const eventTemplate: Mojom.ConversationEntryEvent = {
  completionEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  selectedLanguageEvent: undefined,
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
    sourcesEvent: { sources },
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

const toolEvents: Mojom.ToolUseEvent[] = [
  {
    id: 'abc123d',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: [createTextContentBlock('7:00pm')],
  },
  {
    id: 'abc123e',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: undefined,
  },
  {
    id: 'abc123f',
    toolName: 'user_choice_tool',
    argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
    output: undefined,
  },
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
    smartMode: undefined,
    modelKey: '1',
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
      {
        ...eventTemplate,
        toolUseEvent: {
          id: 'memory-1',
          toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
          argumentsJson: '{"memory": "works as a software engineer"}',
          output: [
            {
              textContentBlock: { text: '' },
              imageContentBlock: undefined,
            },
          ],
        },
      },
      getCompletionEvent("I'll remember that you work as a software engineer."),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
      {
        ...eventTemplate,
        toolUseEvent: {
          id: 'memory-2',
          toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
          argumentsJson: '{"memory": "Likes cats"}',
          output: [
            {
              textContentBlock: { text: '' },
              imageContentBlock: undefined,
            },
          ],
        },
      },
      getCompletionEvent("I've noted you like cats."),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
      {
        ...eventTemplate,
        toolUseEvent: {
          id: 'memory-3',
          toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
          argumentsJson: '{"memory": "favorite hobby is hiking"}',
          output: [
            {
              textContentBlock: { text: 'Memory storage failed' },
              imageContentBlock: undefined,
            },
          ],
        },
      },
      getCompletionEvent(
        'I encountered an error while trying to store that information.',
      ),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
  },
]

const HISTORY: Mojom.ConversationTurn[] = [
  {
    uuid: 'smart-mode-turn',
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
    smartMode: {
      shortcut: 'translate',
      prompt: 'Translate the following text to English',
    },
    modelKey: '1',
  },
  {
    uuid: 'smart-mode-response',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
        smartMode: undefined,
        modelKey: '1',
      },
    ],
    createdTime: { internalValue: BigInt('13278618001000000') },
    events: [],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
    smartMode: undefined,
    modelKey: '1',
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
        'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
      ),
      ...toolEvents
        .slice(0, 3)
        .map((toolUseEvent) => ({ ...eventTemplate, toolUseEvent })),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
      ...toolEvents
        .slice(3)
        .map((toolEvent) => ({ ...eventTemplate, toolUseEvent: toolEvent })),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    smartMode: undefined,
    modelKey: '1',
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
  model: string
  inputText: string
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
  attachmentsDialog: 'tabs' | 'bookmarks' | null
  isNewConversation: boolean
  generatedUrlToBeOpened: Url | undefined
  ratingTurnUuid: { isLiked: boolean; turnUuid: string } | undefined
  isTemporaryChat: boolean
  isDragActive: boolean
  isDragOver: boolean
  useMemoryHistory: boolean
  smartModeDialog: Mojom.SmartMode | null
  selectedActionType: Mojom.ActionType | undefined
  selectedSmartMode: Mojom.SmartMode | undefined
}

const args: CustomArgs = {
  initialized: true,
  inputText: `Write a Star Trek poem about Data's life on board the Enterprise`,
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
  smartModeDialog: null,
  selectedActionType: undefined,
  selectedSmartMode: undefined,
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

function StoryContext(
  props: React.PropsWithChildren<{
    args: CustomArgs
    setArgs: (newArgs: Partial<CustomArgs>) => void
  }>,
) {
  const isSmall = useIsSmall()

  const options = { args: props.args }
  const { setArgs } = props

  const associatedContent = options.args.hasAssociatedContent
    ? ASSOCIATED_CONTENT
    : new Mojom.AssociatedContent()
  const suggestedQuestions = options.args.hasSuggestedQuestions
    ? SAMPLE_QUESTIONS
    : associatedContent
      ? [SAMPLE_QUESTIONS[0]]
      : []

  const currentError = Mojom.APIError[options.args.currentErrorState]
  const apiHasError = currentError !== Mojom.APIError.None
  const currentModel = MODELS.find((m) => m.displayName === options.args.model)

  const switchToBasicModel = () => {
    const nonPremiumModel = MODELS.find(
      (model) =>
        model.options.leoModelOptions?.access === Mojom.ModelAccess.BASIC,
    )
    setArgs({ model: nonPremiumModel?.key })
  }

  const setInputText = (inputText: string) => {
    setArgs({ inputText })
  }

  const [showSidebar, setShowSidebar] = React.useState(isSmall)
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
    smartModes: [
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
    smartModeDialog: options.args.smartModeDialog,
    setSmartModeDialog: () => {},
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
    conversationHistory: options.args.hasConversation
      ? options.args.useMemoryHistory
        ? MEMORY_HISTORY
        : HISTORY
      : [],
    associatedContentInfo: [associatedContent],
    allModels: MODELS,
    currentModel,
    suggestedQuestions,
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
    selectedSmartMode: options.args.selectedSmartMode,
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
    setInputText,
    setCurrentModel: () => {},
    switchToBasicModel,
    generateSuggestedQuestions: () => {},
    dismissLongConversationInfo: () => {},
    retryAPIRequest: () => {},
    handleResetError: () => {},
    handleStopGenerating: async () => {},
    submitInputTextToAPI: () => {},
    resetSelectedActionType: () => setArgs({ selectedActionType: undefined }),
    resetSelectedSmartMode: () => setArgs({ selectedSmartMode: undefined }),
    handleActionTypeClick: (actionType: Mojom.ActionType) => {
      const update: Partial<CustomArgs> = {
        selectedActionType: actionType,
        selectedSmartMode: undefined,
      }

      if (options.args.inputText.startsWith('/')) {
        update.inputText = ''
      }

      setArgs(update)
      setIsToolsMenuOpen(false)
    },
    handleSmartModeClick: (smartMode: any) => {
      setArgs({
        selectedSmartMode: smartMode,
        selectedActionType: undefined,
        inputText: `/${smartMode.shortcut} `,
      })
      setIsToolsMenuOpen(false)
    },
    handleSmartModeEdit: () => {},
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

  const conversationEntriesContext: UntrustedConversationContext = {
    conversationHistory: conversationContext.conversationHistory,
    conversationCapability: Mojom.ConversationCapability.CONTENT_AGENT,
    isGenerating: conversationContext.isGenerating,
    isLeoModel: conversationContext.isCurrentModelLeo,
    contentUsedPercentage: options.args.shouldShowLongPageWarning ? 48 : 100,
    visualContentUsedPercentage: options.args.shouldShowLongVisualContentWarning
      ? 75
      : undefined,
    totalTokens: BigInt(options.args.totalTokens),
    trimmedTokens: BigInt(options.args.trimmedTokens),
    canSubmitUserEntries: !conversationContext.shouldDisableUserInput,
    isMobile: aiChatContext.isMobile,
    allModels: MODELS,
    currentModelKey: currentModel?.key ?? '',
    associatedContent: [associatedContent],
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
  }

  return (
    <AIChatReactContext.Provider value={aiChatContext}>
      <ActiveChatContext.Provider value={activeChatContext}>
        <ConversationReactContext.Provider value={conversationContext}>
          <UntrustedConversationReactContext.Provider
            value={conversationEntriesContext}
          >
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
          .filter((event) => event.toolName !== Mojom.MEMORY_STORAGE_TOOL_NAME)
          .map((event) => (
            <ToolEvent
              key={event.id}
              toolUseEvent={event}
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
