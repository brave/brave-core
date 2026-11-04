/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../../../../components/ai_chat/resources/common/mojom'
import { MockContext } from '../../../../../components/ai_chat/resources/page/state/mock_context'
import { ChatInput } from './chat_input'

const TOOLS_MODEL = {
  key: 'tools-model',
  displayName: 'Tools model',
  isSuggestedModel: true,
  isNearModel: false,
  visionSupport: false,
  supportsTools: true,
  supportedCapabilities: [],
  options: { customModelOptions: undefined, leoModelOptions: undefined },
} as unknown as Mojom.Model

const ISSUES: Mojom.MCPServer = {
  id: 'issues',
  name: 'Issue tracker',
  endpoint: { url: 'https://mcp.example.com/mcp' },
  enabled: true,
  autoAttach: false,
  slug: 'issuetracker',
  hasToken: false,
  tokenUnreadable: false,
}

async function click(element: Element) {
  await act(async () => {
    fireEvent.click(element)
  })
}

describe('ChatInput connectors', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('attaches connectors to the new conversation, and adds them', async () => {
    const associateMCPServer = jest.fn()
    await act(async () => {
      render(
        <MockContext
          mcpHost={{
            getServers: () => Promise.resolve({ servers: [ISSUES] }),
          }}
          uiHandler={{ associateMCPServer }}
          initialState={{
            conversationState: {
              conversationUuid: 'ntp-conversation',
              allModels: [TOOLS_MODEL],
              currentModelKey: TOOLS_MODEL.key,
            },
          }}
          conversationProps={{ isNTPWidget: true }}
        >
          <ChatInput />
        </MockContext>,
      )
    })

    await click(screen.getByTestId('connectors'))
    await waitFor(() =>
      expect(screen.getByTestId('connector-toggle')).toHaveTextContent(
        'Issue tracker',
      ),
    )
    await click(screen.getByTestId('connector-toggle'))
    expect(associateMCPServer).toHaveBeenCalledWith(
      'issues',
      'ntp-conversation',
    )

    await click(screen.getByTestId('add-connector'))
    expect(
      await screen.findByText('CHAT_UI_CONNECTOR_ADD_TITLE'),
    ).toBeInTheDocument()
    expect(screen.getByTestId('connector-name')).toBeInTheDocument()
  })
})
