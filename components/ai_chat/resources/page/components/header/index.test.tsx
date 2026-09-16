// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, act, fireEvent, waitFor } from '@testing-library/react'
import '@testing-library/jest-dom'
import { MockContext } from '../../state/mock_context'
import { clearAllDataForTesting } from '$web-common/api'
import { ConversationHeader } from './index'

const defaultHeaderProps = {
  startSharingConversation: () => {},
  manageSharedConversations: () => {},
}

const sidebarState = {
  isStandalone: false,
  conversationState: { conversationUuid: 'test-conversation' },
}

async function renderHeader(
  children: React.ReactNode,
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(children)
  })
  return result!
}

describe('ConversationHeader', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  it('shows close and open full page buttons during sidebar onboarding without a conversation', async () => {
    const { container } = await renderHeader(
      <MockContext
        initialState={{
          isStandalone: false,
          conversationState: { conversationUuid: '' },
          serviceState: { hasAcceptedAgreement: false },
        }}
      >
        <ConversationHeader {...defaultHeaderProps} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        container.querySelector('[data-testid="close-button"]'),
      ).toBeInTheDocument()
      expect(
        container.querySelector('[data-testid="open-full-page-button"]'),
      ).toBeInTheDocument()
    })
  })

  it('shows close and open full page buttons during sidebar onboarding', async () => {
    const { container } = await renderHeader(
      <MockContext
        initialState={{
          ...sidebarState,
          serviceState: { hasAcceptedAgreement: false },
        }}
      >
        <ConversationHeader {...defaultHeaderProps} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        container.querySelector('[data-testid="close-button"]'),
      ).toBeInTheDocument()
      expect(
        container.querySelector('[data-testid="open-full-page-button"]'),
      ).toBeInTheDocument()
    })
  })

  it('keeps close and open full page buttons after the agreement is accepted', async () => {
    const { container } = await renderHeader(
      <MockContext
        initialState={{
          ...sidebarState,
          serviceState: { hasAcceptedAgreement: true },
        }}
      >
        <ConversationHeader {...defaultHeaderProps} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        container.querySelector('[data-testid="close-button"]'),
      ).toBeInTheDocument()
      expect(
        container.querySelector('[data-testid="open-full-page-button"]'),
      ).toBeInTheDocument()
    })
  })

  it('hides close and open full page buttons in standalone mode', async () => {
    const { container } = await renderHeader(
      <MockContext
        initialState={{
          isStandalone: true,
          conversationState: { conversationUuid: 'test-conversation' },
          serviceState: { hasAcceptedAgreement: false },
        }}
      >
        <ConversationHeader {...defaultHeaderProps} />
      </MockContext>,
    )

    expect(
      container.querySelector('[data-testid="open-full-page-button"]'),
    ).not.toBeInTheDocument()
    expect(
      container.querySelector('[data-testid="close-button"]'),
    ).not.toBeInTheDocument()
  })

  it('calls UI handlers when close and open full page are clicked', async () => {
    const closeUI = jest.fn()
    const openConversationFullPage = jest.fn()

    const { container } = await renderHeader(
      <MockContext
        uiHandler={{ closeUI, openConversationFullPage }}
        initialState={{
          ...sidebarState,
          serviceState: { hasAcceptedAgreement: false },
        }}
      >
        <ConversationHeader {...defaultHeaderProps} />
      </MockContext>,
    )

    await waitFor(() => {
      expect(
        container.querySelector('[data-testid="open-full-page-button"]'),
      ).toBeInTheDocument()
    })

    fireEvent.click(
      container.querySelector('[data-testid="open-full-page-button"]')!,
    )
    fireEvent.click(container.querySelector('[data-testid="close-button"]')!)

    expect(openConversationFullPage).toHaveBeenCalledWith('test-conversation')
    expect(closeUI).toHaveBeenCalled()
  })
})
