// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../../common/mojom'
import { MockContext } from '../../state/mock_context'
import { useAIChat } from '../../state/ai_chat_context'
import ConnectorNotices from '.'

function connector(
  uuid: string,
  title: string,
  mcpServer: Omit<Mojom.AssociatedMCPServer, 'serverId'>,
): Mojom.AssociatedContent {
  return {
    uuid,
    contentType: Mojom.ContentType.MCPServer,
    title,
    contentId: -1,
    url: { url: 'https://mcp.example.com/' },
    contentUsedPercentage: 100,
    conversationTurnUuid: undefined,
    toolsAttached: true,
    mcpServer: { serverId: `${uuid}-server`, ...mcpServer },
  }
}

// Shows which view of the connectors dialog is open.
function DialogProbe() {
  const aiChat = useAIChat()
  return (
    <div data-testid='dialog'>{JSON.stringify(aiChat.connectorsDialog)}</div>
  )
}

async function renderNotices(
  associatedContent: Mojom.AssociatedContent[],
  options: {
    isRequestInProgress?: boolean
    dismissMCPServerNotice?: (uuid: string) => void
  } = {},
) {
  await act(async () => {
    render(
      <MockContext
        initialState={{
          conversationState: {
            associatedContent,
            isRequestInProgress: options.isRequestInProgress ?? false,
          },
        }}
        conversationHandler={
          options.dismissMCPServerNotice
            ? { dismissMCPServerNotice: options.dismissMCPServerNotice }
            : {}
        }
      >
        <ConnectorNotices />
        <DialogProbe />
      </MockContext>,
    )
  })
}

describe('ConnectorNotices', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('says when an answer was written without a connector', async () => {
    await renderNotices([
      connector('slow', 'Slow server', {
        status: Mojom.MCPServerStatus.kNotResponding,
        missedLatestAnswer: true,
      }),
      connector('locked', 'Locked server', {
        status: Mojom.MCPServerStatus.kAuthRequired,
        missedLatestAnswer: true,
      }),
      connector('gone', 'Gone server', {
        status: Mojom.MCPServerStatus.kUnavailable,
        missedLatestAnswer: true,
      }),
    ])

    expect(screen.getAllByTestId('connector-notice')).toHaveLength(3)
    expect(screen.getByRole('status')).toContainElement(
      screen.getAllByTestId('connector-notice')[0],
    )
    expect(
      screen.getByText('CHAT_UI_CONNECTOR_NOTICE_NOT_RESPONDING'),
    ).toBeInTheDocument()
    // An auth error says a token is needed.
    expect(
      screen.getByText('CHAT_UI_CONNECTOR_NOTICE_AUTH_REQUIRED'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('CHAT_UI_CONNECTOR_NOTICE_UNAVAILABLE'),
    ).toBeInTheDocument()
  })

  it('says nothing of connectors that the answer had, or that are back', async () => {
    await renderNotices([
      connector('used', 'Used server', {
        status: Mojom.MCPServerStatus.kNotResponding,
        missedLatestAnswer: false,
      }),
      // Not shown while it's being checked again.
      connector('checking', 'Checking server', {
        status: Mojom.MCPServerStatus.kChecking,
        missedLatestAnswer: true,
      }),
      connector('back', 'Back server', {
        status: Mojom.MCPServerStatus.kAvailable,
        missedLatestAnswer: true,
      }),
    ])

    expect(screen.queryByTestId('connector-notice')).not.toBeInTheDocument()
    // Still there, so that a notice is announced as it appears.
    expect(screen.getByRole('status')).toBeEmptyDOMElement()
  })

  it('waits for the answer to be written', async () => {
    await renderNotices(
      [
        connector('slow', 'Slow server', {
          status: Mojom.MCPServerStatus.kNotResponding,
          missedLatestAnswer: true,
        }),
      ],
      { isRequestInProgress: true },
    )

    expect(screen.queryByTestId('connector-notice')).not.toBeInTheDocument()
  })

  it('opens the connectors dialog to manage them', async () => {
    await renderNotices([
      connector('slow', 'Slow server', {
        status: Mojom.MCPServerStatus.kNotResponding,
        missedLatestAnswer: true,
      }),
    ])
    expect(screen.getByTestId('dialog')).toHaveTextContent('null')

    await act(async () => {
      fireEvent.click(
        screen.getByText('CHAT_UI_CONNECTOR_NOTICE_MANAGE_BUTTON_LABEL'),
      )
    })
    expect(screen.getByTestId('dialog')).toHaveTextContent('{"view":"list"}')
  })

  it('is dismissed in the browser, for every view of the chat', async () => {
    const dismissMCPServerNotice = jest.fn()
    await renderNotices(
      [
        connector('slow', 'Slow server', {
          status: Mojom.MCPServerStatus.kNotResponding,
          missedLatestAnswer: true,
        }),
      ],
      { dismissMCPServerNotice },
    )

    await act(async () => {
      fireEvent.click(screen.getByTitle('CHAT_UI_CLOSE_NOTICE'))
    })
    expect(dismissMCPServerNotice).toHaveBeenCalledWith('slow')
  })
})
