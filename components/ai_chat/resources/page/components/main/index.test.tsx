// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import * as Mojom from '../../../common/mojom'
import Main from './index'

describe('Main', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('closes the conversation list when the open conversation is deleted', async () => {
    const conversationUIObserverRef: {
      current?: Mojom.ConversationUIInterface
    } = {}
    await act(async () => {
      render(
        <MockContext
          conversationUIObserverRef={conversationUIObserverRef}
          initialState={{ serviceState: { hasAcceptedAgreement: true } }}
        >
          <Main />
        </MockContext>,
      )
    })

    fireEvent.click(screen.getByText('CHAT_UI_MENU_CONVERSATION_HISTORY'))
    await waitFor(() => {
      expect(screen.getByTestId('conversations-list')).toBeInTheDocument()
    })

    act(() => {
      conversationUIObserverRef.current!.onConversationDeleted()
    })

    await waitFor(() => {
      expect(screen.queryByTestId('conversations-list')).not.toBeInTheDocument()
    })
  })
})
