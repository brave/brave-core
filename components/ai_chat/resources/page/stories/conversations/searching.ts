// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import { createConversationTurnWithDefaults } from '../../../common/test_data_utils'

const history: Mojom.ConversationTurn[] = [
  createConversationTurnWithDefaults({
    'uuid': 'c64b7702-14a7-4c9e-b12e-c03c78f35f5d',
    'characterType': 0,
    'actionType': 5,
    'text': 'what is the latest macbook rumout',
    'prompt': undefined,
    'selectedText': undefined,
    'events': undefined,
    'createdTime': { 'internalValue': BigInt('13422066642128753') },
    'edits': undefined,
    'uploadedFiles': [],
    'skill': undefined,
    'fromBraveSearchSERP': false,
    'modelKey': undefined,
    'nearVerificationStatus': undefined,
  }),

  createConversationTurnWithDefaults({
    'uuid': '1000d1cc-598f-415c-99bf-425b124f5eb3',
    'characterType': 1,
    'actionType': 1,
    'text': '',
    'prompt': undefined,
    'selectedText': undefined,
    'events': [
      {
        'completionEvent': undefined,
        'searchQueriesEvent': undefined,
        'searchStatusEvent': undefined,
        'conversationTitleEvent': undefined,
        'sourcesEvent': undefined,
        'contentReceiptEvent': undefined,
        'toolUseEvent': {
          'toolName': 'brave_web_search',
          'id': 'tooluse_wNoZdwGQMIU8gxUhYNSycx',
          'argumentsJson':
            '{"query": ["latest MacBook rumors 2025 2026"], "country": "US", "language": "en"}',
          'output': undefined,
          'artifacts': undefined,
          'permissionChallenge': undefined,
          'isServerResult': false,
        },
        'inlineSearchEvent': undefined,
        'deepResearchEvent': undefined,
      },
      {
        'completionEvent': undefined,
        'searchQueriesEvent': undefined,
        'searchStatusEvent': { 'isSearching': true },
        'conversationTitleEvent': undefined,
        'sourcesEvent': undefined,
        'contentReceiptEvent': undefined,
        'toolUseEvent': undefined,
        'inlineSearchEvent': undefined,
        'deepResearchEvent': undefined,
      },
    ],
    'createdTime': { 'internalValue': BigInt('13422066646459906') },
    'edits': undefined,
    'uploadedFiles': undefined,
    'skill': undefined,
    'fromBraveSearchSERP': false,
    'modelKey': undefined,
    'nearVerificationStatus': undefined,
  }),
]

export default history
