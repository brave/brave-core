// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { act, fireEvent, render } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import Main from './index'

// The real header opens the conversations list from a menu; a plain button is
// enough here.
jest.mock('../header', () => ({
  ConversationHeader: (props: {
    setIsConversationsListOpen: (open: boolean) => void
  }) => (
    <button onClick={() => props.setIsConversationsListOpen(true)}>
      open list
    </button>
  ),
}))

jest.mock('../conversations_list', () => ({
  __esModule: true,
  default: (props: {
    coversConversation?: boolean
    setIsConversationsListOpen: (open: boolean) => void
  }) => (
    <button
      data-covers-conversation={props.coversConversation}
      onClick={() => props.setIsConversationsListOpen(false)}
    >
      close list
    </button>
  ),
}))

async function renderMain(isStandalone: boolean) {
  const setClientVisible = jest.fn()
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(
      <MockContext
        initialState={{
          isStandalone,
          serviceState: { hasAcceptedAgreement: true },
        }}
        conversationHandler={{ setClientVisible }}
      >
        <Main />
      </MockContext>,
    )
  })
  const click = (text: string) =>
    act(async () => {
      fireEvent.click(result.getByText(text))
    })
  return { result: result!, setClientVisible, click }
}

describe('Main', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('reports not visible while the side panel list covers it', async () => {
    const { setClientVisible, click } = await renderMain(false)
    await click('open list')
    await click('close list')
    expect(setClientVisible.mock.calls).toEqual([[true], [false], [true]])
  })

  it('tells the side panel list that it covers the conversation', async () => {
    const { result, click } = await renderMain(false)
    await click('open list')
    expect(result.getByText('close list')).toHaveAttribute(
      'data-covers-conversation',
      'true',
    )
  })

  it('stays visible in the full page, where the list is beside it', async () => {
    const { result, setClientVisible, click } = await renderMain(true)
    await click('open list')
    expect(result.queryByText('close list')).toBeNull()
    expect(setClientVisible.mock.calls).toEqual([[true]])
  })
})
