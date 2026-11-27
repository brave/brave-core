// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type ConversationDataJson } from '../../../common/conversation_serialization'

const conversation: ConversationDataJson = {
  'messages': [
    {
      'uuid': 'user-1',
      'text': 'What are some good beginner houseplants?',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'assistant-1',
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001100000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'Great beginner houseplants include:\n\n- **Pothos**: tolerates low light and irregular watering.\n- **Snake plant**: thrives on neglect and needs water only every few weeks.\n- **ZZ plant**: handles low light and drought well.\n- **Spider plant**: grows quickly and is safe for pets.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'user-2',
      'text': 'Which of those do best in a north-facing window?',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001200000',
        },
      },
      'events': [],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'assistant-2',
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001300000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'A north-facing window gets gentle, indirect light, so the **ZZ plant**, **snake plant** and **pothos** are your best bets. The spider plant will survive but may grow more slowly.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': ['thread-2'],
    },
    {
      'uuid': 'user-3',
      'text': 'How often should I water a snake plant?',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001400000',
        },
      },
      'events': [],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'assistant-3',
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001500000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'Water a snake plant every 2–4 weeks, letting the soil dry out completely between waterings. In winter, once a month is usually enough.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': ['thread-3'],
    },
    {
      'uuid': 'user-4',
      'text': 'Thanks!',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001600000',
        },
      },
      'events': [],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'assistant-4',
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001700000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion': "You're welcome! Happy planting.",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': ['thread-1'],
    },
  ],
  'associatedContent': [],
  'title': '',
}

export default conversation
