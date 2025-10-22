// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createTextContentBlock } from '../../../common/content_block'
import * as Mojom from '../../../common/mojom'
import {
  getCompletionEvent,
  getToolUseEvent,
} from '../../../common/test_data_utils'

export const taskConversationEntries: Mojom.ConversationTurn[] = [
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
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.CLICK_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.DRAG_AND_RELEASE_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.MOVE_MOUSE_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.NAVIGATE_TOOL_NAME,
        argumentsJson: '{ "website_url": "https://www.example.com"}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.NAVIGATE_HISTORY_TOOL_NAME,
        argumentsJson: '{ "direction": "back"}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.SCROLL_ELEMENT_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.SELECT_DROPDOWN_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.TYPE_TEXT_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.WAIT_TOOL_NAME,
        argumentsJson: '{}',
        output: [createTextContentBlock('Success')],
      }),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
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
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
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
      getCompletionEvent('What is your answer to this question though?!'),
      getToolUseEvent({
        id: 'abc123e',
        toolName: 'user_choice_tool',
        argumentsJson: JSON.stringify({ choices: ['7:00pm', '8:00pm'] }),
        output: undefined,
      }),
      getToolUseEvent({
        id: 'abc123d',
        toolName: Mojom.SCROLL_ELEMENT_TOOL_NAME,
        argumentsJson: '{}',
        output: undefined,
      }),
      getToolUseEvent({
        id: 'abc123f',
        toolName: Mojom.NAVIGATE_TOOL_NAME,
        argumentsJson: '{ "website_url": "https://www.example2.com"}',
        output: [createTextContentBlock('Success')],
      }),
    ],
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '1',
  },
]
