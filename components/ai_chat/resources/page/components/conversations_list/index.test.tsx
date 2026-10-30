// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, waitFor, within } from '@testing-library/react'
import { MockContext } from '../../state/mock_context'
import { ActiveChatContext } from '../../state/active_chat_context'
import { clearAllDataForTesting } from '$web-common/api'
import ConversationsList from './index'
import * as Mojom from '../../../common/mojom'
import { ConversationGenerationState } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { useAIChat } from '../../state/ai_chat_context'

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

function makeConversation(
  uuid: string,
  title: string,
  workspaceUuids: string[] = [],
): Mojom.Conversation {
  return {
    uuid,
    title,
    updatedTime: { internalValue: BigInt(0) },
    hasContent: true,
    modelKey: undefined,
    totalTokens: BigInt(0),
    trimmedTokens: BigInt(0),
    temporary: false,
    associatedContent: workspaceUuids.map((workspaceUuid) => ({
      uuid: `${uuid}-${workspaceUuid}`,
      contentType: Mojom.ContentType.Workspace,
      title: 'Workspace',
      contentId: 0,
      url: { url: `workspace://${workspaceUuid}` },
      contentUsedPercentage: 100,
      conversationTurnUuid: 'turn-uuid',
      toolsAttached: true,
    })),
  }
}

function renderWithConversations(
  conversations: Mojom.Conversation[],
  isWorkspaceToolsEnabled = true,
) {
  return renderConversationsList(
    <MockContext
      service={{
        getConversations: () => Promise.resolve({ conversations }),
      }}
      initialState={{
        serviceState: { isStoragePrefEnabled: true },
      }}
      aiChatOverrides={{ isWorkspaceToolsEnabled }}
    >
      <ConversationsList />
    </MockContext>,
  )
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

  describe('Recent workspaces', () => {
    // Sorted most recently updated first, as provided by the service.
    const workspaceConversations = [
      makeConversation('c-1', 'Latest in workspace A', ['ws-a']),
      makeConversation('c-2', 'No workspace'),
      makeConversation('c-3', 'Only in workspace B', ['ws-b']),
      makeConversation('c-4', 'Created workspace A', ['ws-a']),
    ]

    it.each([
      ['workspace tools are disabled', workspaceConversations, false],
      ['no conversations have a workspace', mockConversations, true],
    ])('is not shown when %s', async (_, conversations, isEnabled) => {
      await renderWithConversations(conversations, isEnabled)

      await waitFor(() => {
        expect(screen.getAllByTestId('conversation-title').length).toBe(
          conversations.length,
        )
      })
      expect(screen.queryByTestId('recent-workspaces')).not.toBeInTheDocument()
    })

    it('lists the 5 most recent workspaces, labelled by their first conversation', async () => {
      const conversations = [
        ...Array.from({ length: 6 }, (_, i) =>
          makeConversation(`c-${i}`, `Conversation ${i}`, [`ws-${i}`]),
        ),
        makeConversation('c-6', 'Created workspace 0', ['ws-0']),
      ]
      await renderWithConversations(conversations)

      await waitFor(() => {
        expect(screen.getAllByTestId('workspace-item')).toHaveLength(5)
      })
      expect(
        screen.getAllByTestId('workspace-title').map((el) => el.textContent),
      ).toEqual([
        'Created workspace 0',
        'Conversation 1',
        'Conversation 2',
        'Conversation 3',
        'Conversation 4',
      ])
    })

    it('collapses the section and each workspace', async () => {
      await renderWithConversations(workspaceConversations)

      // The section starts expanded.
      const sectionToggle = await screen.findByTestId(
        'recent-workspaces-toggle',
      )
      expect(sectionToggle).toHaveAttribute('aria-expanded', 'true')
      const workspaceA = screen.getAllByTestId('workspace-item')[0]
      const workspaceToggle = within(workspaceA).getByTestId('workspace-toggle')

      // Workspaces start collapsed, and expand to show their conversations.
      expect(workspaceToggle).toHaveAttribute('aria-expanded', 'false')
      expect(
        within(workspaceA).queryAllByTestId('conversation-title'),
      ).toHaveLength(0)
      act(() => workspaceToggle.click())
      expect(workspaceToggle).toHaveAttribute('aria-expanded', 'true')
      expect(
        within(workspaceA)
          .getAllByTestId('conversation-title')
          .map((el) => el.textContent),
      ).toEqual(['Latest in workspace A', 'Created workspace A'])

      act(() => sectionToggle.click())
      expect(sectionToggle).toHaveAttribute('aria-expanded', 'false')
      expect(screen.queryByTestId('workspace-item')).not.toBeInTheDocument()
    })

    it('renaming from the workspace section only shows one rename input', async () => {
      const { container } = await renderWithConversations(
        workspaceConversations,
      )

      const workspaceB = (await screen.findAllByTestId('workspace-item'))[1]
      act(() => within(workspaceB).getByTestId('workspace-toggle').click())
      const link = within(workspaceB)
        .getByTestId('conversation-title')
        .closest('a')!
      act(() => {
        link.dispatchEvent(new MouseEvent('dblclick', { bubbles: true }))
      })

      await waitFor(() => {
        expect(container.querySelectorAll('input[type=text]')).toHaveLength(1)
      })
      expect(workspaceB.querySelector('input[type=text]')).toBeInTheDocument()
    })

    it('new button creates a conversation with the workspace', async () => {
      const createNewConversationWithWorkspace = jest.fn()
      const setIsConversationsListOpen = jest.fn()
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: workspaceConversations }),
          }}
          aiChatOverrides={{ isWorkspaceToolsEnabled: true }}
        >
          <ActiveChatContext.Provider
            value={{
              api: undefined!,
              selectedConversationId: undefined,
              updateSelectedConversationId: () => {},
              createNewConversation: () => {},
              createNewConversationWithWorkspace,
              isMainConversation: false,
              openMainConversation: () => {},
            }}
          >
            <ConversationsList
              setIsConversationsListOpen={setIsConversationsListOpen}
            />
          </ActiveChatContext.Provider>
        </MockContext>,
      )

      const workspaceB = (await screen.findAllByTestId('workspace-item'))[1]
      act(() =>
        within(workspaceB).getByTestId('workspace-new-conversation').click(),
      )

      expect(createNewConversationWithWorkspace).toHaveBeenCalledWith({
        url: 'workspace://ws-b',
      })
      expect(setIsConversationsListOpen).toHaveBeenCalledWith(false)
    })

    it('is hidden while filtering', async () => {
      const { container } = await renderWithConversations(
        workspaceConversations,
      )
      await screen.findByTestId('recent-workspaces')

      act(() => {
        container.querySelector('leo-input')!.dispatchEvent(
          Object.assign(new Event('input', { bubbles: true }), {
            value: 'workspace',
          }),
        )
      })

      await waitFor(() => {
        expect(
          screen.queryByTestId('recent-workspaces'),
        ).not.toBeInTheDocument()
      })
    })
  })

  describe('Generation indicators', () => {
    const CG = ConversationGenerationState

    function WithGeneration({
      entries,
      children,
    }: {
      entries: Array<[string, number]>
      children: React.ReactNode
    }) {
      const Inner = ({ children: inner }: { children: React.ReactNode }) => {
        const aiChat = useAIChat()
        React.useEffect(() => {
          aiChat.api.conversationGenerationState.update(
            () => new Map(entries as any),
          )
        }, [])
        return <>{inner}</>
      }
      return <Inner>{children}</Inner>
    }

    it('shows ProgressRing when generating and not active', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'uuid-2' },
          }}
        >
          <WithGeneration entries={[['uuid-1', CG.kGenerating]]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        const generating = document.querySelectorAll('.generatingIndicator')
        expect(generating.length).toBe(1)
        const completed = document.querySelectorAll('.completedIndicator')
        expect(completed.length).toBe(0)
      })
    })

    it('hides ProgressRing when generating but active', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'uuid-1' },
          }}
        >
          <WithGeneration entries={[['uuid-1', CG.kGenerating]]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        expect(document.querySelectorAll('.generatingIndicator').length).toBe(0)
        expect(document.querySelectorAll('.completedIndicator').length).toBe(0)
      })
    })

    it('shows completed dot when completedNotViewed and not active', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'uuid-2' },
          }}
        >
          <WithGeneration entries={[['uuid-1', CG.kCompletedNotViewed]]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        expect(document.querySelectorAll('.completedIndicator').length).toBe(1)
        expect(document.querySelectorAll('.generatingIndicator').length).toBe(0)
      })
    })

    it('hides completed dot when completedNotViewed but active', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'uuid-1' },
          }}
        >
          <WithGeneration entries={[['uuid-1', CG.kCompletedNotViewed]]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        expect(document.querySelectorAll('.completedIndicator').length).toBe(0)
      })
    })

    it('shows nothing when idle (no entry)', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'uuid-99' },
          }}
        >
          <WithGeneration entries={[]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        expect(document.querySelectorAll('.generatingIndicator').length).toBe(0)
        expect(document.querySelectorAll('.completedIndicator').length).toBe(0)
      })
    })

    it('prefers generating over completed (mutually exclusive)', async () => {
      await renderConversationsList(
        <MockContext
          service={{
            getConversations: () =>
              Promise.resolve({ conversations: mockConversations }),
          }}
          initialState={{
            serviceState: { isStoragePrefEnabled: true },
            conversationState: { conversationUuid: 'other' },
          }}
        >
          <WithGeneration entries={[['uuid-1', CG.kGenerating]]}>
            <ConversationsList />
          </WithGeneration>
        </MockContext>,
      )

      await waitFor(() => {
        expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
      })
      await waitFor(() => {
        expect(document.querySelectorAll('.generatingIndicator').length).toBe(1)
      })
    })
  })
})
