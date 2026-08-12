// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type ConversationDataJson } from '../../../common/conversation_serialization'

const conversation: ConversationDataJson = {
  'messages': [
    {
      'uuid': 'skill-turn',
      'text': '/translate Hello world, how are you today?',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618000900000',
        },
      },
      'events': [],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'skill': {
        'shortcut': 'translate',
        'prompt': 'Translate the following text to English',
      },
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'skill-response',
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618000950000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'Here is the translation:\n\n"Hello world, how are you today?"\n\nThis text is already in English, so no translation is needed.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'uuid': 'turn-uuid',
      'text': 'Summarize this page',
      'characterType': 0,
      'actionType': 2,
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'The ways that animals move are just about as myriad as the animal kingdom itself. They walk, run, swim, crawl, fly and slither — and within each of those categories lies a tremendous number of subtly different movement types. A seagull and a *hummingbird* both have wings, but otherwise their flight techniques and abilities are poles apart. Orcas and **piranhas** both have tails, but they accomplish very different types of swimming. Even a human walking or running is moving their body in fundamentally different ways.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Question that results in a task',
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'According to screenshots, this website compares differences between Juniper Model Y and legacy one. And a lion image.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              "Sure! Here's a table with 5 Marvel characters:\n\n| First Name | Last Name   | Character Name       | First Appearance |\n|------------|-------------|----------------------|------------------|\n| Tony       | Stark      | Iron Man            | 1968              |\n| Steve      | Rogers     | Captain America      | 1941              |\n| Thor       | Odinson    | Thor                 | 1962              |\n| Natasha    | Romanoff   | Black Widow          | 1964              |\n| Peter      | Parker     | Spider-Man           | 1962              |\n\n\n Let me know if you'd like more details!",
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'click_element',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'drag_and_release',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'move_mouse',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'web_page_navigator',
            'argumentsJson': '{ "website_url": "https://www.example.com"}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'navigate_history',
            'argumentsJson': '{ "direction": "back"}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'scroll_element',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'select_dropdown',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'permissionChallenge': {
              'assessment':
                'This is not at all what you asked for. The agent may have been misled by untrusted content.',
              'plan': '',
            },
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'type_text',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'wait',
            'argumentsJson': '{}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123e',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123f',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion': 'What is your answer to this question though?!',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123e',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
        {
          'toolUseEvent': {
            'permissionChallenge': {
              'assessment':
                'This is not at all what you asked for. The agent may have been misled by untrusted content.',
              'plan': '',
            },
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'scroll_element',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc1234e',
            'toolName': 'click_element',
            'argumentsJson': '{}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Success',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123f',
            'toolName': 'web_page_navigator',
            'argumentsJson': '{ "website_url": "https://www.example2.com"}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text':
        'What is pointer compression?\n...and how does it work?\n    - tell me something interesting',
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              "# Title 1\n ## Title 2\n ## **Title 2** using bold that doesn't look different\n### Title 3\n#### Title 4\n \nDuring the latter part of 2021, I reflected on the challenges we were facing at Modern Health. One recurring problem that stood out was our struggle to create new products with an unstructured color palette. This resulted in poor [communication](https://www.google.com) between designers and developers, an inconsistent product brand, and increasing accessibility problems.\n\n1. Inclusivity: our palette provides easy ways to ensure our product uses accessible contrasts.\n 2. Efficiency: our palette is diverse enough for our current and future product design, yet values are still predictable and constrained.\n 3. Reusability: our palette is on-brand but versatile. There are very few one-offs that fall outside the palette.\n\n This article shares the process I followed to apply these principles to develop a more adaptable color palette that prioritizes accessibility and is built to scale into all of our future product **design** needs.",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'What is taylor series?',
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'The partial sum formed by the first n + 1 terms of a Taylor series is a polynomial of degree n that is called the nth Taylor polynomial of the function. Taylor polynomials are approximations of a function, which become generally better as n increases.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Write a hello world program in c++',
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              "Sure! Here's a table with 5 Marvel characters:\n\n| First Name | Last Name   | Character Name       | First Appearance |\n|------------|-------------|----------------------|------------------|\n| Tony       | Stark      | Iron Man            | 1968              |\n| Steve      | Rogers     | Captain America      | 1941              |\n| Thor       | Odinson    | Thor                 | 1962              |\n| Natasha    | Romanoff   | Black Widow          | 1964              |\n| Peter      | Parker     | Spider-Man           | 1962              |\n\n\n Let me know if you'd like more details!",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Shorten this selected text',
      'characterType': 0,
      'actionType': 17,
      'selectedText':
        'Pointer compression is a memory optimization technique where pointers are stored in a compressed format to save memory.',
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'searchStatusEvent': {
            'isSearching': true,
          },
        },
        {
          'searchQueriesEvent': {
            'searchQueries': [
              'pointer compression',
              'c++ language specification',
            ],
          },
        },
        {
          'completionEvent': {
            'completion':
              '[1]:https://www.example.com\n[2]:https://lttstore.com\n[3]:https://www.tesla.com/modely\n[Pointer compression](https://www.example.com) is a [memory](https://brave.com/wont-show-as-link) optimization technique.[1][3]',
          },
        },
        {
          'sourcesEvent': {
            'sources': [
              {
                'url': {
                  'url': 'https://www.example.com',
                },
                'title': 'Pointer Compression',
                'faviconUrl': {
                  'url': 'https://www.example.com/favicon.ico',
                },
              },
              {
                'title': 'LTT Store',
                'faviconUrl': {
                  'url': 'https://lttstore.com/favicon.ico',
                },
                'url': {
                  'url': 'https://lttstore.com',
                },
              },
              {
                'title': 'Tesla Model Y',
                'faviconUrl': {
                  'url': 'https://www.tesla.com/favicon.ico',
                },
                'url': {
                  'url': 'https://www.tesla.com/modely',
                },
              },
            ],
            'richResults': [],
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Will an LTT store backpack fit in a Tesla Model Y frunk?',
      'characterType': 0,
      'actionType': 17,
      'selectedText': '',
      'edits': [
        {
          'text': 'Will it fit in a Tesla Model Y frunk?',
          'characterType': 0,
          'actionType': 17,
          'selectedText': '',
          'createdTime': {
            'internalValue': {
              '$bigint': '13278618001000000',
            },
          },
          'edits': [],
          'events': [],
          'uploadedFiles': [],
          'fromBraveSearchSERP': false,
          'modelKey': '1',
          'childThreadUuids': [],
        },
      ],
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
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'searchStatusEvent': {
            'isSearching': true,
          },
        },
        {
          'searchQueriesEvent': {
            'searchQueries': [
              'LTT store backpack dimensions',
              'Tesla Model Y frunk dimensions',
            ],
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'What is this image?',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [],
      'uploadedFiles': [
        {
          'filename': 'lion.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 0,
        },
      ],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion': 'It is a lion!',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Summarize this page',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [],
      'uploadedFiles': [
        {
          'filename': 'full_screenshot_0.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 1,
        },
        {
          'filename': 'full_screenshot_1.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 1,
        },
      ],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'This website compares differences between Juniper Model Y and legacy one.',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': 'Stored, refer to input for data',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'assistant_detail_storage',
            'argumentsJson':
              '{"information":"This is some data that the LLM wants to store for later before other tool use responses get removed from context because they are too large"}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': 'Summarize these',
      'characterType': 0,
      'actionType': 5,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [],
      'uploadedFiles': [
        {
          'filename': 'full_screenshot_0.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 1,
        },
        {
          'filename': 'full_screenshot_1.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 1,
        },
        {
          'filename': 'lion.png',
          'filesize': 128,
          'data': {
            '$bytes':
              'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
          },
          'type': 0,
        },
      ],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              'According to screenshots, this website compares differences between Juniper Model Y and legacy one. And a lion image.',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion':
              "Sure! Here's a table with 5 Marvel characters:\n\n| First Name | Last Name   | Character Name       | First Appearance |\n|------------|-------------|----------------------|------------------|\n| Tony       | Stark      | Iron Man            | 1968              |\n| Steve      | Rogers     | Captain America      | 1941              |\n| Thor       | Odinson    | Thor                 | 1962              |\n| Natasha    | Romanoff   | Black Widow          | 1964              |\n| Peter      | Parker     | Spider-Man           | 1962              |\n\n\n Let me know if you'd like more details!",
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123f',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123g',
            'toolName': 'web_page_navigator',
            'argumentsJson': '{"website_url":"https://www.example.com"}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123h',
            'toolName': 'code_execution_tool',
            'argumentsJson':
              '{"script":"const result = 1 + 2;\\nreturn `1 + 2 = ${result};`"}',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': '1 + 2 = 3',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123i',
            'toolName': 'code_execution_tool',
            'argumentsJson':
              '{"script":"const result = 1 + 2;\\nreturn `1 + 2 = ${result}`;"}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
    {
      'text': '',
      'characterType': 1,
      'actionType': 0,
      'edits': [],
      'createdTime': {
        'internalValue': {
          '$bigint': '13278618001000000',
        },
      },
      'events': [
        {
          'completionEvent': {
            'completion': 'Answer one of these questions:',
          },
        },
        {
          'toolUseEvent': {
            'output': [
              {
                'textContentBlock': {
                  'text': '7:00pm',
                },
              },
            ],
            'isServerResult': false,
            'id': 'abc123d',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123e',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
        {
          'toolUseEvent': {
            'isServerResult': false,
            'id': 'abc123f',
            'toolName': 'user_choice_tool',
            'argumentsJson': '{"choices":["7:00pm","8:00pm"]}',
          },
        },
      ],
      'uploadedFiles': [],
      'fromBraveSearchSERP': false,
      'modelKey': '1',
      'childThreadUuids': [],
    },
  ],
  'associatedContent': [
    {
      'uuid': 'uuid',
      'contentType': 0,
      'title': 'Tiny Tweaks to Neurons Can Rewire Animal Motion',
      'contentUsedPercentage': 40,
      'url': {
        'url':
          'https://www.example.com/areallylongurlthatwillbetruncatedintheinputbox',
      },
      'contentId': 1,
      'conversationTurnUuid': 'turn-uuid',
      'toolsAttached': false,
    },
  ],
  'title':
    'Sorting C++ vectors is hard especially when you have to have a very long title for your conversation to test text clipping or wrapping',
}

export default conversation
