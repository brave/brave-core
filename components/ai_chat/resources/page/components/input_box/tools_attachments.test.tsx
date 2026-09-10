// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { MockContext } from '../../state/mock_context'
import ToolsAttachments from './tools_attachments'
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
  { name: 'browse_store', description: 'Browse OR navigate to collections.' },
  { name: 'cancel_cart', description: 'Remove all items from the cart.' },
]

async function renderToolsAttachments(tools: Mojom.ToolInfo[]) {
  const getContentTools = jest.fn(() => Promise.resolve({ tools }))
  const setToolsAttached = jest.fn()
  await act(async () => {
    render(
      <MockContext conversationHandler={{ getContentTools }}>
        <ToolsAttachments
          toolsContent={[CONTENT]}
          setToolsAttached={setToolsAttached}
        />
      </MockContext>,
    )
  })
  return { getContentTools, setToolsAttached }
}

describe('ToolsAttachments', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  it('counts the tools the site provides', async () => {
    await renderToolsAttachments(TOOLS)

    expect(await screen.findByText('2')).toBeInTheDocument()
    expect(
      screen.getByLabelText('CHAT_UI_WEBSITE_TOOLS_LIST_LABEL'),
    ).toBeInTheDocument()
  })

  it('omits the count when the site provides no tools', async () => {
    const { getContentTools } = await renderToolsAttachments([])

    // Wait for the empty answer, so this isn't just asserting the state before
    // the tools were fetched.
    await waitFor(() =>
      expect(getContentTools).toHaveBeenCalledWith('content-uuid'),
    )
    expect(
      screen.getByRole('button', { name: 'My Website' }),
    ).toBeInTheDocument()
    expect(
      screen.queryByLabelText('CHAT_UI_WEBSITE_TOOLS_LIST_LABEL'),
    ).not.toBeInTheDocument()
  })

  it('opens the tools dialog when the pill is clicked', async () => {
    await renderToolsAttachments(TOOLS)

    expect(screen.queryByText('CHAT_UI_WEBSITE_TOOLS_TITLE')).toBeNull()

    await act(async () => {
      fireEvent.click(screen.getByRole('button', { name: /My Website/ }))
    })

    expect(screen.getByText('CHAT_UI_WEBSITE_TOOLS_TITLE')).toBeInTheDocument()
  })

  it('detaches the tools when the pill is removed', async () => {
    const { setToolsAttached } = await renderToolsAttachments(TOOLS)

    fireEvent.click(document.querySelector('leo-button.toolPillRemove')!)

    expect(setToolsAttached).toHaveBeenCalledWith(CONTENT, false)
  })
})
