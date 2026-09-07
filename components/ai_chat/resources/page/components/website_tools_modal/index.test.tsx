// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import WebsiteToolsModal from './index'
import * as Mojom from '../../../common/mojom'

const CONTENT: Mojom.AssociatedContent = {
  uuid: 'content-uuid',
  contentType: Mojom.ContentType.PageContent,
  title: 'My Website',
  contentId: 1,
  url: { url: 'https://mywebsite.com/collections/water' },
  contentUsedPercentage: 100,
  conversationTurnUuid: undefined,
  toolsAttached: true,
}

const TOOLS: Mojom.ToolInfo[] = [
  {
    name: 'browse_store',
    description: 'Browse OR navigate to collections.',
    permission: Mojom.ToolPermission.kAsk,
  },
  {
    name: 'cancel_cart',
    description: 'Remove all items from the cart.',
    permission: Mojom.ToolPermission.kNeverAllow,
  },
]

async function renderModal(ui: React.ReactElement) {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(ui)
  })
  return result!
}

describe('WebsiteToolsModal', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  it('lists the tools provided by the content it was given', async () => {
    const getContentTools = jest.fn(() => Promise.resolve({ tools: TOOLS }))

    await renderModal(
      <MockContext conversationHandler={{ getContentTools }}>
        <WebsiteToolsModal
          content={CONTENT}
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByText('browse_store')).toBeInTheDocument()
    })
    expect(getContentTools).toHaveBeenCalledWith('content-uuid')
    expect(screen.getByText('cancel_cart')).toBeInTheDocument()
    expect(
      screen.getByText('Browse OR navigate to collections.'),
    ).toBeInTheDocument()
    // Granting tools lets the site see the conversation, so it must be
    // identifiable.
    expect(screen.getByText('My Website')).toBeInTheDocument()
    expect(
      screen.getByText('mywebsite.com/collections/water'),
    ).toBeInTheDocument()
  })

  it('shows each tool the permission it currently has', async () => {
    const { container } = await renderModal(
      <MockContext
        conversationHandler={{
          getContentTools: () => Promise.resolve({ tools: TOOLS }),
        }}
      >
        <WebsiteToolsModal
          content={CONTENT}
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(container.querySelectorAll('leo-dropdown')).toHaveLength(2)
    })
    const dropdowns = container.querySelectorAll('leo-dropdown')
    expect(dropdowns[0]).toHaveProperty(
      'value',
      String(Mojom.ToolPermission.kAsk),
    )
    expect(dropdowns[1]).toHaveProperty(
      'value',
      String(Mojom.ToolPermission.kNeverAllow),
    )
  })

  it('records the permission the user picks for a tool', async () => {
    const setContentToolPermission = jest.fn()

    const { container } = await renderModal(
      <MockContext
        conversationHandler={{
          getContentTools: () => Promise.resolve({ tools: TOOLS }),
          setContentToolPermission,
        }}
      >
        <WebsiteToolsModal
          content={CONTENT}
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(container.querySelectorAll('leo-dropdown')).toHaveLength(2)
    })

    await act(async () => {
      container.querySelectorAll('leo-dropdown')[0].dispatchEvent(
        Object.assign(new Event('change', { bubbles: true }), {
          value: String(Mojom.ToolPermission.kAlwaysAllow),
        }),
      )
    })

    expect(setContentToolPermission).toHaveBeenCalledWith(
      'content-uuid',
      'browse_store',
      Mojom.ToolPermission.kAlwaysAllow,
    )
  })

  it('shows a permission change the browser pushed', async () => {
    // The choice is conversation-wide, so a dialog open in another view has to
    // reflect it too - without asking the page for its tools again, which
    // could answer with a different list mid-interaction.
    const getContentTools = jest.fn(() => Promise.resolve({ tools: TOOLS }))
    const observerRef: { current?: Mojom.ConversationUIInterface } = {}

    const { container } = await renderModal(
      <MockContext
        conversationHandler={{ getContentTools }}
        conversationUIObserverRef={observerRef}
      >
        <WebsiteToolsModal
          content={CONTENT}
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(container.querySelectorAll('leo-dropdown')).toHaveLength(2)
    })

    await act(async () => {
      observerRef.current!.onContentToolsChanged('content-uuid', [
        { ...TOOLS[0], permission: Mojom.ToolPermission.kAlwaysAllow },
        TOOLS[1],
      ])
    })

    await waitFor(() => {
      expect(container.querySelectorAll('leo-dropdown')[0]).toHaveProperty(
        'value',
        String(Mojom.ToolPermission.kAlwaysAllow),
      )
    })
    expect(getContentTools).toHaveBeenCalledTimes(1)
  })

  it.each([
    // The loop reads the choices once, when it starts, so a change made part
    // way through wouldn't apply until the next one.
    ['a response is generating', { isRequestInProgress: true }],
    // The request is over by the time the tools run, so it's the task state
    // that says the loop is still in flight - including while it waits on a
    // permission challenge.
    [
      'a tool loop is running',
      {
        isRequestInProgress: false,
        toolUseTaskState: Mojom.TaskState.kRunning,
      },
    ],
    [
      'a tool loop is paused',
      { isRequestInProgress: false, toolUseTaskState: Mojom.TaskState.kPaused },
    ],
  ])(
    'locks the permission pickers while %s',
    async (_, conversationState: Partial<Mojom.ConversationState>) => {
      const { container } = await renderModal(
        <MockContext
          conversationHandler={{
            getContentTools: () => Promise.resolve({ tools: TOOLS }),
          }}
          initialState={{ conversationState }}
        >
          <WebsiteToolsModal
            content={CONTENT}
            onClose={() => {}}
          />
        </MockContext>,
      )

      await waitFor(() => {
        expect(container.querySelectorAll('leo-dropdown')).toHaveLength(2)
      })
      for (const dropdown of container.querySelectorAll('leo-dropdown')) {
        expect(dropdown).toHaveProperty('disabled', true)
      }
    },
  )

  it.each([
    ['no tool loop has started', Mojom.TaskState.kNone],
    ['the tool loop was stopped', Mojom.TaskState.kStopped],
  ])(
    'leaves the permission pickers usable when %s',
    async (_, toolUseTaskState: Mojom.TaskState) => {
      const { container } = await renderModal(
        <MockContext
          conversationHandler={{
            getContentTools: () => Promise.resolve({ tools: TOOLS }),
          }}
          initialState={{
            conversationState: { isRequestInProgress: false, toolUseTaskState },
          }}
        >
          <WebsiteToolsModal
            content={CONTENT}
            onClose={() => {}}
          />
        </MockContext>,
      )

      await waitFor(() => {
        expect(container.querySelectorAll('leo-dropdown')).toHaveLength(2)
      })
      for (const dropdown of container.querySelectorAll('leo-dropdown')) {
        expect(dropdown).toHaveProperty('disabled', false)
      }
    },
  )

  it('counts the tools it lists', async () => {
    await renderModal(
      <MockContext
        conversationHandler={{
          getContentTools: () => Promise.resolve({ tools: TOOLS }),
        }}
      >
        <WebsiteToolsModal
          content={CONTENT}
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        screen.getByText('CHAT_UI_WEBSITE_TOOLS_LIST_LABEL'),
      ).toBeInTheDocument()
    })
  })
})
