// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, waitFor } from '@testing-library/react'
import { MockContext } from '../../state/mock_context'
import DeleteConversationModal from './index'

function modal(deletingConversationId: string | null) {
  return (
    <MockContext aiChatOverrides={{ deletingConversationId }}>
      <DeleteConversationModal />
    </MockContext>
  )
}

describe('DeleteConversationModal', () => {
  it('focuses Cancel, not Delete, when the dialog opens', async () => {
    let result: ReturnType<typeof render>
    await act(async () => {
      result = render(modal(null))
    })

    const cancel = screen.getByText('CHAT_UI_CANCEL_BUTTON_LABEL')
    // Custom elements are disabled in jest, so the button isn't focusable as
    // it is in the browser (via delegatesFocus).
    cancel.tabIndex = 0
    expect(cancel).not.toHaveFocus()

    await act(async () => {
      result!.rerender(modal('uuid-1'))
    })

    await waitFor(() => expect(cancel).toHaveFocus())
  })
})
