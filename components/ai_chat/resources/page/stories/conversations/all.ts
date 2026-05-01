// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import { taskConversationEntries } from '../story_utils/history'
import { toolUseCompleteAssistantDetailStorage } from '../story_utils/events'
import {
  getCompletionEvent,
  getSearchQueriesEvent,
  getSearchStatusEvent,
  getToolUseEvent,
  getWebSourcesEvent,
} from '../../../common/test_data_utils'
import { createTextContentBlock } from '../../../common/content_block'

export const toolEvents: Mojom.ConversationEntryEvent[] = [
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
      getSearchQueriesEvent([
        'pointer compression',
        'c++ language specification',
      ]),
      getCompletionEvent(
        '[1]:https://www.example.com\n[2]:https://lttstore.com\n[3]:https://www.tesla.com/modely\n[Pointer compression](https://www.example.com) is a [memory](https://brave.com/wont-show-as-link) optimization technique.[1][3]',
      ),
      getWebSourcesEvent([
        {
          url: { url: 'https://www.example.com' },
          title: 'Pointer Compression',
          faviconUrl: { url: 'https://www.example.com/favicon.ico' },
          pageContent: undefined,
          extraSnippets: undefined,
        },
        {
          title: 'LTT Store',
          faviconUrl: { url: 'https://lttstore.com/favicon.ico' },
          url: { url: 'https://lttstore.com' },
          pageContent: undefined,
          extraSnippets: undefined,
        },
        {
          title: 'Tesla Model Y',
          faviconUrl: { url: 'https://www.tesla.com/favicon.ico' },
          url: { url: 'https://www.tesla.com/modely' },
          pageContent: undefined,
          extraSnippets: undefined,
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
      getSearchQueriesEvent([
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
        extractedText: undefined,
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
        extractedText: undefined,
      },
      {
        filename: 'full_screenshot_1.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
        extractedText: undefined,
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
        extractedText: undefined,
      },
      {
        filename: 'full_screenshot_1.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kScreenshot,
        extractedText: undefined,
      },
      {
        filename: 'lion.png',
        filesize: 128,
        data: Array.from(new Uint8Array(128)),
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
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

export default HISTORY
