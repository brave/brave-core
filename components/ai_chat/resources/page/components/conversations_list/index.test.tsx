// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import {
  act,
  fireEvent,
  render,
  screen,
  waitFor,
  within,
} from '@testing-library/react'
import { MockContext } from '../../state/mock_context'
import { clearAllDataForTesting } from '$web-common/api'
import ConversationsList from './index'
import * as Mojom from '../../../common/mojom'

const mockConversations: Mojom.Conversation[] = [
  {
    uuid: 'uuid-1',
    title: 'How to use TypeScript',
    updatedTime: { internalValue: BigInt(0) },
    hasContent: true,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
    associatedContent: [],
  },
  {
    uuid: 'uuid-2',
    title: 'Brave browser features',
    updatedTime: { internalValue: BigInt(0) },
    hasContent: true,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
    associatedContent: [],
  },
  {
    uuid: 'uuid-3',
    title: 'React performance tips',
    updatedTime: { internalValue: BigInt(0) },
    hasContent: true,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
    associatedContent: [],
  },
]

async function renderConversationsList(
  ...args: Parameters<typeof render>
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(...args)
  })
  return result!
}

describe('ConversationsList', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  describe('Rendering', () => {
    it('renders all conversations when storage pref is enabled', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
        expect(screen.getByText('Brave browser features')).toBeInTheDocument()
        expect(screen.getByText('React performance tips')).toBeInTheDocument()
      })
    })

    it('shows empty history notice when storage pref is enabled but no conversations', async () => {
      await renderConversationsList(
        <MockContext
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(
          screen.getByText('CHAT_UI_NOTICE_CONVERSATION_HISTORY_EMPTY'),
        ).toBeInTheDocument()
      })
    })

    it('shows disabled storage pref notice when storage pref is disabled', async () => {
      await renderConversationsList(
        <MockContext
          initialState={{
            serviceState: { isStoragePrefEnabled: false },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(
          screen.getByText(
            'CHAT_UI_NOTICE_CONVERSATION_HISTORY_TITLE_DISABLED_PREF',
          ),
        ).toBeInTheDocument()
      })
    })

    it('does not render the filter input when there are no conversations', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(container.querySelector('leo-input')).not.toBeInTheDocument()
      })
    })

    it('renders the filter input when there are conversations', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(container.querySelector('leo-input')).toBeInTheDocument()
      })
    })
  })

  describe('Filter', () => {
    it('filters conversations by title', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })

      // Simulate the Leo input's custom 'input' event with a value property
      const leoInput = container.querySelector('leo-input')!
      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), {
            value: 'brave',
          }),
        )
      })

      await waitFor(() => {
        expect(screen.getByText('Brave browser features')).toBeInTheDocument()
        expect(
          screen.queryByText('How to use TypeScript'),
        ).not.toBeInTheDocument()
        expect(
          screen.queryByText('React performance tips'),
        ).not.toBeInTheDocument()
      })
    })

    it('filter is case-insensitive', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })

      const leoInput = container.querySelector('leo-input')!
      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), {
            value: 'TYPESCRIPT',
          }),
        )
      })

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
        expect(
          screen.queryByText('Brave browser features'),
        ).not.toBeInTheDocument()
      })
    })

    it('shows all conversations when filter is cleared', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })

      const leoInput = container.querySelector('leo-input')!

      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), {
            value: 'brave',
          }),
        )
      })

      await waitFor(() => {
        expect(
          screen.queryByText('How to use TypeScript'),
        ).not.toBeInTheDocument()
      })

      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), { value: '' }),
        )
      })

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
        expect(screen.getByText('Brave browser features')).toBeInTheDocument()
        expect(screen.getByText('React performance tips')).toBeInTheDocument()
      })
    })

    it('shows no-results state when filter matches nothing', async () => {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })

      const leoInput = container.querySelector('leo-input')!
      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), {
            value: 'xyzzy',
          }),
        )
      })

      await waitFor(() => {
        expect(
          screen.getByText('AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS'),
        ).toBeInTheDocument()
        expect(
          screen.getByText(
            'AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS_SUBTITLE',
          ),
        ).toBeInTheDocument()
      })
    })

    it('does not show no-results state when filter is empty', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )

      await waitFor(() => {
        expect(
          screen.queryByText('AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS'),
        ).not.toBeInTheDocument()
      })
    })
  })

  describe('Conversation search', () => {
    async function renderWithConversationSearch(
      searchConversations: Mojom.AIChatUIHandlerInterface['searchConversations'],
    ) {
      const { container } = await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          uiHandler={{ searchConversations }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
          }}
        >
          <ConversationsList />
        </MockContext>,
      )
      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      return container
    }

    function search(container: HTMLElement, value: string) {
      const leoInput = container.querySelector('leo-input')!
      act(() => {
        leoInput.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), { value }),
        )
      })
    }

    it('shows conversations that match by content in their own section', async () => {
      const searchConversations = jest.fn(async (query: string) => ({
        matches:
          query === 'block trackers'
            ? [
                {
                  conversationUuid: 'uuid-2',
                  snippet: 'Shields block trackers by default.',
                  entryUuid: 'entry-7',
                },
              ]
            : [],
      }))
      const container = await renderWithConversationSearch(searchConversations)

      search(container, 'block trackers')

      const section = await screen.findByTestId('conversation-search-results')
      expect(
        within(section).getByText(
          'AI_CHAT_CONVERSATION_LIST_CONVERSATION_SEARCH_HEADING',
        ),
      ).toBeInTheDocument()
      expect(
        within(section).getByText('Brave browser features'),
      ).toBeInTheDocument()
      expect(
        within(section).getByText('Shields block trackers by default.'),
      ).toBeInTheDocument()
      expect(searchConversations).toHaveBeenCalledWith('block trackers')
      // No title matches, but a conversation did match by content.
      expect(
        screen.queryByText('AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS'),
      ).not.toBeInTheDocument()
    })

    describe('card', () => {
      let pushState: jest.SpyInstance

      beforeEach(() => {
        pushState = jest
          .spyOn(window.history, 'pushState')
          .mockImplementation(() => {})
      })

      afterEach(() => {
        pushState.mockRestore()
      })

      async function renderCard(entryUuid: string | undefined) {
        const container = await renderWithConversationSearch(async () => ({
          matches: [
            {
              conversationUuid: 'uuid-2',
              snippet: 'Shields block trackers by default.',
              entryUuid,
            },
          ],
        }))
        search(container, 'block trackers')
        return screen.findByTestId('conversation-search-item')
      }

      it('opens the conversation at the entry the passage is from', async () => {
        const card = await renderCard('entry-7')
        expect(card).toHaveAttribute('href', '/uuid-2')

        fireEvent.click(card)

        expect(pushState).toHaveBeenCalledTimes(1)
        const url = pushState.mock.calls[0][2] as string
        expect(url.startsWith('/uuid-2#entry=entry-7&n=')).toBe(true)
      })

      it('names the entry afresh for each click', async () => {
        const card = await renderCard('entry-7')
        const now = jest.spyOn(Date, 'now')
        now.mockReturnValueOnce(1000).mockReturnValueOnce(2000)

        fireEvent.click(card)
        fireEvent.click(card)

        expect(pushState.mock.calls[0][2]).not.toBe(pushState.mock.calls[1][2])
        now.mockRestore()
      })

      it('opens the conversation at its start when only its title matched', async () => {
        const card = await renderCard(undefined)

        fireEvent.click(card)

        expect(pushState).toHaveBeenCalledWith(null, '', '/uuid-2')
      })

      it('leaves clicks that open another tab to the browser', async () => {
        const card = await renderCard('entry-7')

        fireEvent.click(card, { ctrlKey: true })

        expect(pushState).not.toHaveBeenCalled()
      })
    })

    it('leaves out conversations that are no longer listed', async () => {
      const container = await renderWithConversationSearch(async () => ({
        matches: [{ conversationUuid: 'deleted', snippet: 'Gone' }],
      }))

      search(container, 'block trackers')

      await waitFor(() => {
        expect(
          screen.getByText('AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS'),
        ).toBeInTheDocument()
      })
      expect(
        screen.queryByTestId('conversation-search-results'),
      ).not.toBeInTheDocument()
    })

    it('shows no section where conversation search is unavailable', async () => {
      const searchConversations = jest.fn(async () => ({ matches: null }))
      const container = await renderWithConversationSearch(searchConversations)

      search(container, 'block trackers')

      await waitFor(() => {
        expect(searchConversations).toHaveBeenCalledWith('block trackers')
      })
      expect(
        screen.getByText('AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS'),
      ).toBeInTheDocument()
      expect(
        screen.queryByTestId('conversation-search-results'),
      ).not.toBeInTheDocument()
    })
  })
})
