// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type ConversationDataJson } from '../../../common/conversation_serialization'

const conversation: ConversationDataJson = {
  'messages': [
    {
      'uuid': 'user-1',
      'text': 'Remember that I work as a software engineer.',
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
      threadUuid: undefined,
      childThreadUuids: [],
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
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': '',
                },
              },
            ],
            'isServerResult': false,
            'id': 'memory-1',
            'toolName': 'memory_storage_tool',
            'argumentsJson': '{"memory": "works as a software engineer"}',
          },
        },
        {
          'completionEvent': {
            'completion': "I'll remember that you work as a software engineer.",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      threadUuid: undefined,
      childThreadUuids: [],
    },
    {
      'uuid': 'user-2',
      'text': 'Remember I like cats.',
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
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': '',
                },
              },
            ],
            'isServerResult': false,
            'id': 'memory-2',
            'toolName': 'memory_storage_tool',
            'argumentsJson': '{"memory": "Likes cats"}',
          },
        },
        {
          'completionEvent': {
            'completion': "I've noted you like cats.",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'user-3',
      'text': 'Remember my favorite hobby is hiking.',
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
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Memory storage failed',
                },
              },
            ],
            'isServerResult': false,
            'id': 'memory-3',
            'toolName': 'memory_storage_tool',
            'argumentsJson': '{"memory": "favorite hobby is hiking"}',
          },
        },
        {
          'completionEvent': {
            'completion':
              'I encountered an error while trying to store that information.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
  ],
  'associatedContent': [],
  'title': '',
}

export default conversation
