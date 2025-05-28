// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import * as Mojom from '../../../common/mojom'
import AssistantResponse from '.'

const eventTemplate: Mojom.ConversationEntryEvent = {
  completionEvent: undefined,
  pageContentRefineEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  selectedLanguageEvent: undefined,
  conversationTitleEvent: undefined,
  sourcesEvent: undefined,
  contentReceiptEvent: undefined,
}

function getCompletionEvent(text: string): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    completionEvent: { completion: text },
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
      entry={testEntry}
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
