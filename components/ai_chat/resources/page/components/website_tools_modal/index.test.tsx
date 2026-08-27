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
  { name: 'browse_store', description: 'Browse OR navigate to collections.' },
  { name: 'cancel_cart', description: 'Remove all items from the cart.' },
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
