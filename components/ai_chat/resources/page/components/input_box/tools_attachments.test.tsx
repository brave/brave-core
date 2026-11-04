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

const MCP_SERVER: Mojom.AssociatedContent = {
  ...CONTENT,
  uuid: 'server-uuid',
  contentType: Mojom.ContentType.MCPServer,
  title: 'My Server',
  contentId: -1,
  url: { url: 'https://mcp.example.com/mcp' },
}

const TOOLS: Mojom.ToolInfo[] = [
  { name: 'browse_store', description: 'Browse OR navigate to collections.' },
  { name: 'cancel_cart', description: 'Remove all items from the cart.' },
]

async function renderToolsAttachments(
  tools: Mojom.ToolInfo[],
  content: Mojom.AssociatedContent = CONTENT,
) {
  const getContentTools = jest.fn(() => Promise.resolve({ tools }))
  const setToolsAttached = jest.fn()
  const disassociateContent = jest.fn()
  const reconnectServer = jest.fn()
  const conversationHandler = { getContentTools }
  const mcpHost = { reconnectServer }
  const getElement = (content: Mojom.AssociatedContent) => (
    <MockContext
      conversationHandler={conversationHandler}
      mcpHost={mcpHost}
    >
      <ToolsAttachments
        toolsContent={[content]}
        setToolsAttached={setToolsAttached}
        disassociateContent={disassociateContent}
      />
    </MockContext>
  )
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(getElement(content))
  })
  return {
    getContentTools,
    setToolsAttached,
    disassociateContent,
    reconnectServer,
    rerender: (content: Mojom.AssociatedContent) =>
      act(async () => result.rerender(getElement(content))),
  }
}

function getConnector(status: Mojom.MCPServerStatus): Mojom.AssociatedContent {
  return {
    ...MCP_SERVER,
    mcpServer: { serverId: 'server-id', status, missedLatestAnswer: false },
  }
}

function getIconNames(element: Element): string[] {
  return Array.from(
    element.querySelectorAll('leo-icon'),
    (icon) => (icon as any).name,
  )
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

  it('does not open the tools dialog for a workspace', async () => {
    const workspace: Mojom.AssociatedContent = {
      ...CONTENT,
      contentType: Mojom.ContentType.Workspace,
      title: 'Workspace',
      url: { url: 'workspace://abc' },
    }
    const { setToolsAttached } = await renderToolsAttachments(TOOLS, workspace)

    expect(await screen.findByText('2')).toBeInTheDocument()
    expect(screen.queryByRole('button', { name: /Workspace/ })).toBeNull()

    await act(async () => {
      fireEvent.click(screen.getAllByText('Workspace')[0])
    })
    expect(screen.queryByText('CHAT_UI_WEBSITE_TOOLS_TITLE')).toBeNull()

    // It can still be removed.
    fireEvent.click(document.querySelector('leo-button.toolPillRemove')!)
    expect(setToolsAttached).toHaveBeenCalledWith(workspace, false)
  })

  it('shows an MCP server with an icon rather than a favicon', async () => {
    await renderToolsAttachments(TOOLS, MCP_SERVER)

    expect(await screen.findByText('2')).toBeInTheDocument()
    expect(document.querySelector('img[src*="//favicon2"]')).toBeNull()
    expect(document.querySelector('.toolPillIcon')).toBeInTheDocument()

    await act(async () => {
      fireEvent.click(screen.getByRole('button', { name: /My Server/ }))
    })
    expect(
      screen.getByText('CHAT_UI_MCP_SERVER_TOOLS_TITLE'),
    ).toBeInTheDocument()
    expect(screen.getByText('CHAT_UI_MCP_SERVER_LABEL')).toBeInTheDocument()
    expect(document.querySelector('img[src*="//favicon2"]')).toBeNull()
  })

  it('detaches the tools when the pill is removed', async () => {
    const { setToolsAttached } = await renderToolsAttachments(TOOLS)

    fireEvent.click(document.querySelector('leo-button.toolPillRemove')!)

    expect(setToolsAttached).toHaveBeenCalledWith(CONTENT, false)
  })

  it('detaches an MCP server when its pill is removed', async () => {
    const { setToolsAttached, disassociateContent } =
      await renderToolsAttachments(TOOLS, MCP_SERVER)

    fireEvent.click(document.querySelector('leo-button.toolPillRemove')!)

    expect(disassociateContent).toHaveBeenCalledWith(MCP_SERVER)
    expect(setToolsAttached).not.toHaveBeenCalled()
  })

  describe('connector status', () => {
    const Status = Mojom.MCPServerStatus

    it('shows nothing more once a connector is available', async () => {
      const { rerender } = await renderToolsAttachments(
        TOOLS,
        getConnector(Status.kNotResponding),
      )
      expect(screen.getByTestId('connector-status')).toBeInTheDocument()
      expect(screen.getByTestId('connector-reconnect')).toBeInTheDocument()

      await rerender(getConnector(Status.kAvailable))
      expect(screen.getByText('2')).toBeInTheDocument()
      expect(screen.queryByTestId('connector-status')).toBeNull()
      expect(screen.queryByTestId('connector-reconnect')).toBeNull()
    })

    it('shows a connector that is not responding as a warning', async () => {
      const { reconnectServer } = await renderToolsAttachments(
        TOOLS,
        getConnector(Status.kNotResponding),
      )

      const status = screen.getByTestId('connector-status')
      expect(status).toHaveTextContent(
        'CHAT_UI_CONNECTOR_STATUS_NOT_RESPONDING',
      )
      expect(getIconNames(status)).toEqual(['warning-triangle-outline'])
      // Says why, and what's being done about it, to keyboard and touch too.
      const description = document.getElementById(
        status.getAttribute('aria-describedby')!,
      )
      expect(description).toHaveTextContent(
        'CHAT_UI_CONNECTOR_STATUS_NOT_RESPONDING_DESCRIPTION',
      )
      expect(description).toHaveAttribute('slot', 'content')
      expect(status).toHaveAttribute('tabindex', '0')

      fireEvent.click(screen.getByTestId('connector-reconnect'))
      expect(reconnectServer).toHaveBeenCalledWith('server-id')
    })

    it.each([
      [Status.kAuthRequired, 'AUTH_REQUIRED'],
      [Status.kUnavailable, 'UNAVAILABLE'],
    ])(
      'offers no reconnect where it would not help (%s)',
      async (status, name) => {
        await renderToolsAttachments(TOOLS, getConnector(status))

        const element = screen.getByTestId('connector-status')
        expect(element).toHaveTextContent(`CHAT_UI_CONNECTOR_STATUS_${name}`)
        expect(getIconNames(element)).toEqual(['warning-triangle-outline'])
        expect(
          document.getElementById(element.getAttribute('aria-describedby')!),
        ).toHaveTextContent(`CHAT_UI_CONNECTOR_STATUS_${name}_DESCRIPTION`)
        expect(screen.queryByTestId('connector-reconnect')).toBeNull()
      },
    )

    it('can reconnect to a connector that is not supported', async () => {
      const { reconnectServer } = await renderToolsAttachments(
        TOOLS,
        getConnector(Status.kUnsupported),
      )

      expect(screen.getByTestId('connector-status')).toHaveTextContent(
        'CHAT_UI_CONNECTOR_STATUS_UNSUPPORTED',
      )
      fireEvent.click(screen.getByTestId('connector-reconnect'))
      expect(reconnectServer).toHaveBeenCalledWith('server-id')
    })

    it('shows that it is checking a connector again', async () => {
      await renderToolsAttachments(TOOLS, getConnector(Status.kChecking))

      const status = screen.getByTestId('connector-status')
      expect(status).toHaveTextContent('CHAT_UI_CONNECTOR_STATUS_CONNECTING')
      expect(status.querySelector('leo-progressring')).toBeInTheDocument()
      expect(getIconNames(status)).toEqual([])
      expect(status).not.toHaveAttribute('tabindex')
      expect(screen.queryByTestId('connector-reconnect')).toBeNull()
    })
  })
})
