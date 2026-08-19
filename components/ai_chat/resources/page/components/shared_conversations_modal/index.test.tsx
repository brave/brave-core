// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, waitFor, fireEvent } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import SharedConversationsModal from './index'
import * as Mojom from '../../../common/mojom'

const showAlert = jest.fn()
jest.mock('@brave/leo/react/alertCenter', () => ({
  __esModule: true,
  default: () => null,
  showAlert: (...args: unknown[]) => showAlert(...args),
}))

// 2026-02-03 09:15 UTC, expressed as microseconds since the Windows epoch.
const sharedTime = {
  internalValue:
    BigInt(
      Date.UTC(2026, 1, 3, 9, 15)
        + (Date.UTC(1970, 0, 1) - Date.UTC(1601, 0, 1)),
    ) * BigInt(1000),
}

const mockShares: Mojom.ConversationShare[] = [
  {
    shareId: 'share-id-1',
    conversationUuid: 'conversation-uuid-1',
    conversationTitle: 'How to use TypeScript',
    createdTime: sharedTime,
  },
  {
    shareId: 'share-id-2',
    conversationUuid: 'conversation-uuid-2',
    conversationTitle: '',
    createdTime: sharedTime,
  },
]

async function renderModal(ui: React.ReactElement) {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(ui)
  })
  return result!
}

describe('SharedConversationsModal', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  it('lists each share with its title, time and share id', async () => {
    await renderModal(
      <MockContext
        service={{
          getConversationShares: () => Promise.resolve({ shares: mockShares }),
        }}
      >
        <SharedConversationsModal onClose={() => {}} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
    })
    expect(screen.getByText('share-id-1')).toBeInTheDocument()
    expect(screen.getByText('share-id-2')).toBeInTheDocument()
    // A conversation which was never titled still needs to be identifiable.
    expect(
      screen.getByText('AI_CHAT_CONVERSATION_LIST_UNTITLED'),
    ).toBeInTheDocument()
    // Shown in the user's locale and time zone, so build the expectation the
    // same way rather than hard-coding a formatted string.
    const expectedTime = new Intl.DateTimeFormat(undefined, {
      month: 'short',
      day: 'numeric',
      year: 'numeric',
      hour: 'numeric',
      minute: 'numeric',
    }).format(new Date(Date.UTC(2026, 1, 3, 9, 15)))
    expect(screen.getAllByText(expectedTime)).toHaveLength(2)
  })

  it('shows an empty state when nothing has been shared', async () => {
    await renderModal(
      <MockContext>
        <SharedConversationsModal onClose={() => {}} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        screen.getByText('CHAT_UI_SHARED_CONVERSATIONS_DIALOG_EMPTY'),
      ).toBeInTheDocument()
    })
  })

  it('deletes the share it was asked to and drops it from the list', async () => {
    let shares = [...mockShares]
    const deleteConversationShare = jest.fn((shareId: string) => {
      shares = shares.filter((share) => share.shareId !== shareId)
      return Promise.resolve({ success: true })
    })

    await renderModal(
      <MockContext
        service={{
          getConversationShares: () => Promise.resolve({ shares }),
          deleteConversationShare,
        }}
      >
        <SharedConversationsModal
          isOpen
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByText('How to use TypeScript')).toBeInTheDocument()
    })

    await act(async () => {
      fireEvent.click(
        screen.getAllByTitle(
          'CHAT_UI_SHARED_CONVERSATIONS_DELETE_BUTTON_LABEL',
        )[0],
      )
    })

    expect(deleteConversationShare).toHaveBeenCalledWith('share-id-1')
    await waitFor(() => {
      expect(screen.queryByText('share-id-1')).not.toBeInTheDocument()
    })
    expect(screen.getByText('share-id-2')).toBeInTheDocument()
    expect(showAlert).not.toHaveBeenCalled()
  })

  it('keeps the share and alerts when the server refuses to delete it', async () => {
    await renderModal(
      <MockContext
        service={{
          getConversationShares: () => Promise.resolve({ shares: mockShares }),
          deleteConversationShare: () => Promise.resolve({ success: false }),
        }}
      >
        <SharedConversationsModal
          isOpen
          onClose={() => {}}
        />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByText('share-id-1')).toBeInTheDocument()
    })

    await act(async () => {
      fireEvent.click(
        screen.getAllByTitle(
          'CHAT_UI_SHARED_CONVERSATIONS_DELETE_BUTTON_LABEL',
        )[0],
      )
    })

    // The share still exists on the server, so it must stay listed for a retry.
    expect(screen.getByText('share-id-1')).toBeInTheDocument()
    expect(showAlert).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'error',
        content: 'CHAT_UI_SHARED_CONVERSATIONS_DELETE_ERROR',
      }),
    )
  })

  it('asks the browser to copy the link, rather than copying it itself', async () => {
    const copyConversationShareLink = jest.fn()

    await renderModal(
      <MockContext
        service={{
          getConversationShares: () => Promise.resolve({ shares: mockShares }),
          copyConversationShareLink,
        }}
      >
        <SharedConversationsModal onClose={() => {}} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(screen.getByText('share-id-1')).toBeInTheDocument()
    })

    await act(async () => {
      fireEvent.click(
        screen.getAllByTitle(
          'CHAT_UI_SHARED_CONVERSATIONS_COPY_LINK_BUTTON_LABEL',
        )[0],
      )
    })

    // Only the browser process holds the decryption key for a past share, and
    // only it can mark the clipboard entry confidential.
    expect(copyConversationShareLink).toHaveBeenCalledWith('share-id-1')
  })
})
