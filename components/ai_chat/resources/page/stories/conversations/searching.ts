// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type ConversationDataJson } from '../../../common/conversation_serialization'

const conversation: ConversationDataJson = {
  'messages': [
    {
      'uuid': 'c64b7702-14a7-4c9e-b12e-c03c78f35f5d',
      'createdTime': {
        'internalValue': {
          '$bigint': '13422066642128753',
        },
      },
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'actionType': 5,
      'characterType': 0,
      'text': 'what is the latest macbook rumout',
      'childThreadUuids': [],
    },
    {
      'uuid': '1000d1cc-598f-415c-99bf-425b124f5eb3',
      'events': [
        {
          'toolUseEvent': {
            'toolName': 'brave_web_search',
            'id': 'tooluse_wNoZdwGQMIU8gxUhYNSycx',
            'argumentsJson':
              '{"query": ["latest MacBook rumors 2025 2026"], "country": "US", "language": "en"}',
            'isServerResult': false,
          },
        },
        {
          'searchStatusEvent': {
            'isSearching': true,
          },
        },
      ],
      'createdTime': {
        'internalValue': {
          '$bigint': '13422066646459906',
        },
      },
      'fromBraveSearchSERP': false,
      'actionType': 1,
      'characterType': 1,
      'text': '',
      'childThreadUuids': [],
    },
  ],
  'associatedContent': [],
  'title': '',
}

export default conversation
