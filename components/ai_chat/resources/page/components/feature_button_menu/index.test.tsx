// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import FeatureMenu from './index'

async function renderMenu(
  aiChatOverrides: Parameters<typeof MockContext>[0]['aiChatOverrides'],
) {
  await act(async () => {
    render(
      <MockContext aiChatOverrides={aiChatOverrides}>
        <FeatureMenu manageSharedConversations={() => {}} />
      </MockContext>,
    )
  })
}

describe('FeatureMenu', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('opens the list of connectors', async () => {
    const setConnectorsDialog = jest.fn()
    await renderMenu({ isMCPEnabled: true, setConnectorsDialog })

    await act(async () => {
      fireEvent.click(screen.getByText('CHAT_UI_MENU_MANAGE_CONNECTORS'))
    })

    expect(setConnectorsDialog).toHaveBeenCalledWith({ view: 'list' })
  })

  it('offers no connectors while the feature is off', async () => {
    await renderMenu({ isMCPEnabled: false })

    expect(
      screen.queryByText('CHAT_UI_MENU_MANAGE_CONNECTORS'),
    ).not.toBeInTheDocument()
  })
})
