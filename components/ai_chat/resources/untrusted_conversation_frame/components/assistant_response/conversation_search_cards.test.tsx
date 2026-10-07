// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import * as React from 'react'
import { fireEvent, render, screen } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import ConversationSearchCards, {
  parseConversationCards,
} from './conversation_search_cards'

function makeArtifact(contentJson: string, type?: string): Mojom.ToolArtifact {
  return {
    id: null,
    type: type ?? Mojom.CONVERSATION_SEARCH_RESULTS_ARTIFACT_TYPE,
    contentJson,
  }
}

const cards = [
  {
    conversation_uuid: 'uuid-1',
    title: 'Rust lifetimes',
    entry_uuid: 'entry-1',
    snippet: 'Lifetimes say how long a borrow is valid.',
  },
  { conversation_uuid: 'uuid-2', title: 'Cats' },
]

describe('parseConversationCards', () => {
  it('reads the cards', () => {
    expect(parseConversationCards(makeArtifact(JSON.stringify(cards)))).toEqual(
      [
        {
          conversationUuid: 'uuid-1',
          title: 'Rust lifetimes',
          entryUuid: 'entry-1',
          snippet: 'Lifetimes say how long a borrow is valid.',
        },
        {
          conversationUuid: 'uuid-2',
          title: 'Cats',
          entryUuid: undefined,
          snippet: undefined,
        },
      ],
    )
  })

  it('reads nothing from anything else', () => {
    expect(parseConversationCards(makeArtifact('not json'))).toEqual([])
    expect(parseConversationCards(makeArtifact('{"a": 1}'))).toEqual([])
    expect(parseConversationCards(makeArtifact('[]', 'line_chart'))).toEqual([])
  })

  it('leaves out what is not a card', () => {
    const parsed = parseConversationCards(
      makeArtifact(
        JSON.stringify([
          null,
          'text',
          { title: 'No uuid' },
          { conversation_uuid: '', title: 'Empty uuid' },
          { conversation_uuid: 'uuid-3' },
          { conversation_uuid: 'uuid-4', title: 'Fine', entry_uuid: 7 },
        ]),
      ),
    )
    expect(parsed).toEqual([
      {
        conversationUuid: 'uuid-4',
        title: 'Fine',
        entryUuid: undefined,
        snippet: undefined,
      },
    ])
  })
})

describe('ConversationSearchCards', () => {
  it('shows a card for each conversation, with the passage that matched', () => {
    render(
      <MockContext>
        <ConversationSearchCards
          artifact={makeArtifact(JSON.stringify(cards))}
        />
      </MockContext>,
    )

    const shown = screen.getAllByTestId('conversation-search-card')
    expect(shown).toHaveLength(2)
    expect(shown[0]).toHaveTextContent('Rust lifetimes')
    expect(shown[0]).toHaveTextContent(
      'Lifetimes say how long a borrow is valid.',
    )
    // A conversation found by its title has no passage to show.
    expect(shown[1]).toHaveTextContent('Cats')
    // Nothing but the cards is rendered.
    expect(screen.getByTestId('conversation-search-cards').textContent).toBe(
      shown.map((card) => card.textContent).join(''),
    )
    expect(
      screen.getAllByTestId('conversation-search-card-snippet'),
    ).toHaveLength(1)
  })

  it('opens the conversation at the entry that matched', () => {
    const openConversation = jest.fn()
    render(
      <MockContext parentUIFrame={{ openConversation }}>
        <ConversationSearchCards
          artifact={makeArtifact(JSON.stringify(cards))}
        />
      </MockContext>,
    )

    const shown = screen.getAllByTestId('conversation-search-card')
    fireEvent.click(shown[0])
    expect(openConversation).toHaveBeenLastCalledWith('uuid-1', 'entry-1')

    fireEvent.click(shown[1])
    expect(openConversation).toHaveBeenLastCalledWith('uuid-2', null)
  })

  it('shows nothing when there is nothing to show', () => {
    render(
      <MockContext>
        <ConversationSearchCards artifact={makeArtifact('[]')} />
      </MockContext>,
    )

    expect(
      screen.queryByTestId('conversation-search-cards'),
    ).not.toBeInTheDocument()
  })
})
