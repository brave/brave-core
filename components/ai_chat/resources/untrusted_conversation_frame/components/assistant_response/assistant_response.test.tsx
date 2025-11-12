// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Set up loadTimeData mock BEFORE importing components
;(window as any).loadTimeData = {
  getString: jest.fn((key: string) => {
    if (key === 'richSearchWidgetsOrigin') {
      return 'https://example.com'
    }
    return ''
  }),
  getStringF: jest.fn().mockReturnValue(''),
  getInteger: jest.fn().mockReturnValue(0),
  getBoolean: jest.fn().mockReturnValue(false),
  set: jest.fn(),
  data_: {},
}

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen, waitFor } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import * as Mojom from '../../../common/mojom'
import {
  getCompletionEvent,
  getWebSourcesEvent,
} from '../../../common/test_data_utils'
import { createTextContentBlock } from '../../../common/content_block'
import MockContext from '../../mock_untrusted_conversation_context'
import AssistantResponse from '.'

test('AssistantResponse should include expandable sources', async () => {
  const testEntry: Mojom.ConversationTurn = {
    uuid: 'test-uuid',
    characterType: Mojom.CharacterType.ASSISTANT,
    text: 'test text',
    actionType: Mojom.ActionType.RESPONSE,
    prompt: undefined,
    selectedText: undefined,
    createdTime: { internalValue: BigInt('13278618001000000') },
    edits: undefined,
    fromBraveSearchSERP: false,
    uploadedFiles: undefined,
    modelKey: undefined,
    events: [
      getCompletionEvent('test completion'),
      getWebSourcesEvent([
        {
          title: 'Source 1',
          faviconUrl: { url: 'https://imgs.example.com/favicon1.ico' },
          url: { url: 'https://1.example.com/path' },
        },
        {
          title: 'Source 2',
          faviconUrl: { url: 'https://imgs.example.com/favicon2.ico' },
          url: { url: 'https://2.example.com/path' },
        },
        {
          title: 'Source 3',
          faviconUrl: { url: 'https://imgs.example.com/favicon3.ico' },
          url: { url: 'https://3.example.com/path' },
        },
        {
          title: 'Source 4',
          faviconUrl: { url: 'https://imgs.example.com/favicon4.ico' },
          url: { url: 'https://4.example.com/path' },
        },
        {
          title: 'Source 5',
          faviconUrl: { url: 'https://imgs.example.com/favicon5.ico' },
          url: { url: 'https://5.example.com/path' },
        },
        {
          title: 'Source 6',
          faviconUrl: { url: 'https://imgs.example.com/favicon6.ico' },
          url: { url: 'https://6.example.com/path' },
        },
        {
          title: 'Source 7',
          faviconUrl: { url: 'https://imgs.example.com/favicon7.ico' },
          url: { url: 'https://7.example.com/path' },
        },
        {
          title: 'Source 8',
          faviconUrl: { url: 'https://imgs.example.com/favicon8.ico' },
          url: { url: 'https://8.example.com/path' },
        },
      ]),
    ],
  }
  render(
    <AssistantResponse
      events={testEntry.events!}
      isEntryInteractivityAllowed={false}
      isLeoModel={true}
      isEntryInProgress={false}
      allowedLinks={[]}
    />,
  )
  // There should be the first items showing
  let links = screen.getAllByRole('link')
  expect(links).toHaveLength(4)
  // The expand button should be visible
  const expandButton = screen.getByRole('button')
  await userEvent.click(expandButton)
  // There should be all items showing
  links = screen.getAllByRole('link')
  expect(links).toHaveLength(8)
})

test('AssistantResponse should render memory tool events inline', async () => {
  const mockHasMemory = () => Promise.resolve({ exists: true })
  const mockUIObserver = {
    onMemoriesChanged: {
      addListener: jest.fn().mockReturnValue('listener-id'),
    },
    removeListener: jest.fn(),
  }

  const memoryToolEvent: Mojom.ConversationEntryEvent = {
    toolUseEvent: {
      toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
      id: 'memory-tool-123',
      argumentsJson: '{"memory": "Test memory content"}',
      output: [createTextContentBlock('')],
    },
  }

  const events = [memoryToolEvent, getCompletionEvent('I will remember that.')]

  render(
    <MockContext
      uiHandler={
        {
          hasMemory: mockHasMemory,
        } as unknown as Mojom.UntrustedUIHandlerRemote
      }
      uiObserver={mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter}
    >
      <AssistantResponse
        events={events}
        isEntryInteractivityAllowed={false}
        isLeoModel={true}
        isEntryInProgress={false}
        allowedLinks={[]}
      />
    </MockContext>,
  )

  // Memory tool should render inline (success state)
  await waitFor(() => {
    expect(screen.getByTestId('memory-tool-event')).toBeInTheDocument()
  })
  expect(
    screen.getByText(/CHAT_UI_MEMORY_UPDATED_WITH_CONTENT_LABEL/),
  ).toBeInTheDocument()
})

test(
  'AssistantResponse should render memory tool in undone state when memory '
    + 'does not exist',
  async () => {
    const mockHasMemory = () => Promise.resolve({ exists: false })

    const memoryToolEvent: Mojom.ConversationEntryEvent = {
      toolUseEvent: {
        toolName: Mojom.MEMORY_STORAGE_TOOL_NAME,
        id: 'memory-tool-123',
        argumentsJson: '{"memory": "Test memory content"}',
        output: [createTextContentBlock('')],
      },
    }

    const events = [
      memoryToolEvent,
      getCompletionEvent('I will remember that.'),
    ]

    render(
      <MockContext
        uiHandler={
          {
            hasMemory: mockHasMemory,
          } as unknown as Mojom.UntrustedUIHandlerRemote
        }
      >
        <AssistantResponse
          events={events}
          isEntryInteractivityAllowed={false}
          isLeoModel={true}
          isEntryInProgress={false}
          allowedLinks={[]}
        />
      </MockContext>,
    )

    // Memory tool should render in undone state when memory doesn't exist
    await waitFor(() => {
      expect(screen.getByTestId('memory-tool-event-undone')).toBeInTheDocument()
    })
    expect(screen.queryByTestId('memory-tool-event')).not.toBeInTheDocument()
  },
)

test('AssistantResponse should not render iframe when richResults is null or empty', () => {
  const events = [
    getCompletionEvent('test completion'),
    getWebSourcesEvent([
      {
        title: 'Source 1',
        faviconUrl: { url: 'https://imgs.example.com/favicon1.ico' },
        url: { url: 'https://1.example.com/path' },
      },
    ]),
  ]

  const { container } = render(
    <AssistantResponse
      events={events}
      isEntryInteractivityAllowed={false}
      isLeoModel={true}
      isEntryInProgress={false}
      allowedLinks={[]}
    />,
  )

  // Should not render any iframes when richResults is empty
  const iframes = container.querySelectorAll('iframe')
  expect(iframes).toHaveLength(0)
})

test('AssistantResponse should render iframe when richResults has data', () => {
  // Note: This data is made up and doesn't reflect what the actual widgets look like.
  // We don't test the actual widget rendering here because it can change out of band.
  const richResultsData = [
    '{"type":"weather","location":"San Francisco","temp":72}',
    '{"type":"stock","symbol":"AAPL","price":150.50}',
    null,
    undefined,
    '',
  ]

  const events = [
    getCompletionEvent('test completion'),
    getWebSourcesEvent(
      [
        {
          title: 'Source 1',
          faviconUrl: { url: 'https://imgs.example.com/favicon1.ico' },
          url: { url: 'https://1.example.com/path' },
        },
      ],
      // Note: We cast this to a string[] to double check that we're filtering out falsey values.
      richResultsData as string[],
    ),
  ]

  const { container } = render(
    <AssistantResponse
      events={events}
      isEntryInteractivityAllowed={false}
      isLeoModel={true}
      isEntryInProgress={false}
      allowedLinks={[]}
    />,
  )

  // Should render iframes for each richResult
  const iframes = container.querySelectorAll('iframe')
  expect(iframes).toHaveLength(2)
})
