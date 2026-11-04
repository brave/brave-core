// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../../common/mojom'
import { MockContext, MockContextProps } from '../../state/mock_context'
import AttachmentButtonMenu from '.'

jest.mock('$web-common/uploadFile', () => ({
  pickFiles: jest.fn(),
}))

// eslint-disable-next-line import/first
import { pickFiles } from '$web-common/uploadFile'
const mockPickFiles = pickFiles as jest.MockedFunction<typeof pickFiles>

const defaultProps = {
  attachFiles: jest.fn(),
  getScreenshots: jest.fn(),
  conversationHistory: [],
  associatedContentInfo: [],
  setAttachmentsDialog: jest.fn(),
  associateDefaultContent: undefined,
  unassociatedTabs: [],
  isMobile: false,
  conversationStarted: false,
  focusInput: jest.fn(),
}

// Mirrors the accept constant in the component — must match its preprocessor block exactly.
const expectedAccept: string | undefined =
  // <if expr="is_android">
  'image/*,application/pdf'
// <else>
undefined
// </if>

const MCP_SERVER_CONTENT: Mojom.AssociatedContent = {
  uuid: 'server-uuid',
  contentType: Mojom.ContentType.MCPServer,
  title: 'Attached Server',
  contentId: -1,
  url: { url: 'https://mcp.example.com/mcp' },
  contentUsedPercentage: 100,
  conversationTurnUuid: undefined,
  toolsAttached: true,
  mcpServer: undefined,
}

const PAGE_CONTENT: Mojom.AssociatedContent = {
  uuid: 'page-uuid',
  contentType: Mojom.ContentType.PageContent,
  title: 'Attached Page',
  contentId: 1,
  url: { url: 'https://example.com/' },
  contentUsedPercentage: 100,
  conversationTurnUuid: undefined,
  toolsAttached: false,
  mcpServer: undefined,
}

describe('attachment button menu', () => {
  beforeEach(() => {
    mockPickFiles.mockResolvedValue([])
  })

  it('upload menu item passes correct accept to pickFiles', async () => {
    const { container } = render(<AttachmentButtonMenu {...defaultProps} />)

    const menuItems = container.querySelectorAll<HTMLElement>('leo-menu-item')
    await act(async () => {
      menuItems[0].click()
    })

    expect(mockPickFiles).toHaveBeenCalledWith({
      multiple: true,
      accept: expectedAccept,
    })
  })

  it('camera menu item passes correct accept to pickFiles', async () => {
    const { container } = render(
      <AttachmentButtonMenu
        {...defaultProps}
        isMobile
      />,
    )

    // With isMobile=true and no associated content, menu order is:
    // [0] upload, [1] camera, [2] bookmarks, [3] history
    const menuItems = container.querySelectorAll<HTMLElement>('leo-menu-item')
    await act(async () => {
      menuItems[1].click()
    })

    expect(mockPickFiles).toHaveBeenCalledWith({
      capture: 'environment',
      accept: expectedAccept,
    })
  })

  it('offers a screenshot of a page attached beside an MCP server', async () => {
    const { container } = render(
      <AttachmentButtonMenu
        {...defaultProps}
        associatedContentInfo={[PAGE_CONTENT, MCP_SERVER_CONTENT]}
      />,
    )
    expect(
      container.querySelector('leo-menu-item[data-testid="screenshot"]'),
    ).toBeInTheDocument()
  })

  it('offers no screenshot of an MCP server', async () => {
    const { container } = render(
      <AttachmentButtonMenu
        {...defaultProps}
        associatedContentInfo={[MCP_SERVER_CONTENT]}
      />,
    )
    expect(
      container.querySelector('leo-menu-item[data-testid="screenshot"]'),
    ).toBeNull()
  })
})

const ISSUES: Mojom.MCPServer = {
  id: 'issues',
  name: 'Issue tracker',
  endpoint: { url: 'https://mcp.example.com/secret-key/mcp' },
  enabled: true,
  autoAttach: false,
  slug: 'issuetracker',
  hasToken: false,
  tokenUnreadable: false,
}

const DOCS: Mojom.MCPServer = {
  ...ISSUES,
  id: 'docs',
  name: 'Docs',
  endpoint: { url: 'https://docs.example.com/mcp' },
  slug: 'docs',
}

const CALENDAR: Mojom.MCPServer = {
  ...ISSUES,
  id: 'calendar',
  name: 'Calendar',
  slug: 'calendar',
  hasToken: true,
  tokenUnreadable: true,
}

const NOTES: Mojom.MCPServer = {
  ...ISSUES,
  id: 'notes',
  name: 'Local notes',
  slug: 'localnotes',
  enabled: false,
}

const ISSUES_CONTENT: Mojom.AssociatedContent = {
  uuid: 'issues-content',
  contentType: Mojom.ContentType.MCPServer,
  title: 'Issue tracker',
  contentId: -1,
  url: { url: 'https://mcp.example.com/' },
  contentUsedPercentage: 100,
  conversationTurnUuid: undefined,
  toolsAttached: true,
  mcpServer: {
    serverId: 'issues',
    status: Mojom.MCPServerStatus.kAvailable,
    missedLatestAnswer: false,
  },
}

const CONVERSATION_UUID = 'conversation-uuid'

interface Options {
  associatedContent?: Mojom.AssociatedContent[]
  canAttachConnectors?: boolean
  mcpHostObserverRef?: MockContextProps['mcpHostObserverRef']
  uiHandler?: MockContextProps['uiHandler']
  conversationHandler?: MockContextProps['conversationHandler']
  setConnectorsDialog?: jest.Mock
}

async function renderMenu(options: Options = {}) {
  const associatedContent = options.associatedContent ?? [ISSUES_CONTENT]
  await act(async () => {
    render(
      <MockContext
        mcpHost={{
          getServers: () =>
            Promise.resolve({ servers: [ISSUES, CALENDAR, NOTES, DOCS] }),
        }}
        uiHandler={options.uiHandler}
        conversationHandler={options.conversationHandler}
        mcpHostObserverRef={options.mcpHostObserverRef}
        aiChatOverrides={
          options.setConnectorsDialog
            ? { setConnectorsDialog: options.setConnectorsDialog }
            : undefined
        }
        initialState={{
          conversationState: {
            conversationUuid: CONVERSATION_UUID,
            associatedContent,
          },
        }}
      >
        <AttachmentButtonMenu
          attachFiles={jest.fn()}
          getScreenshots={jest.fn()}
          conversationHistory={[]}
          associatedContentInfo={associatedContent}
          setAttachmentsDialog={jest.fn()}
          associateDefaultContent={undefined}
          unassociatedTabs={[]}
          isMobile={false}
          conversationStarted={false}
          focusInput={jest.fn()}
          canAttachConnectors={options.canAttachConnectors ?? true}
        />
      </MockContext>,
    )
  })
}

async function click(element: Element) {
  await act(async () => {
    fireEvent.click(element)
  })
}

async function openConnectors() {
  await click(screen.getByTestId('connectors'))
  await waitFor(() =>
    expect(screen.getByTestId('connectors-back')).toBeInTheDocument(),
  )
}

function getToggle(name: string) {
  const item = screen
    .getAllByTestId('connector-toggle')
    .find((toggle) => toggle.textContent === name)
  if (!item) {
    throw new Error(`No toggle for ${name}`)
  }
  return item
}

describe('attachment button menu connectors', () => {
  // Run after each test, whether or not it passed.
  let cleanups: Array<() => void> = []

  beforeEach(() => {
    clearAllDataForTesting()
  })

  afterEach(() => {
    for (const cleanup of cleanups) {
      cleanup()
    }
    cleanups = []
  })

  it("has no connectors where they can't be attached", async () => {
    await renderMenu({ canAttachConnectors: false })
    expect(screen.queryByTestId('connectors')).not.toBeInTheDocument()
  })

  it('shows a toggle for each connector that is turned on', async () => {
    await renderMenu()
    await openConnectors()

    expect(
      screen.getAllByTestId('connector-toggle').map((item) => item.textContent),
    ).toEqual(['Issue tracker', 'Docs'])
    expect(
      (getToggle('Issue tracker').querySelector('leo-toggle') as any).checked,
    ).toBe(true)
    expect((getToggle('Docs').querySelector('leo-toggle') as any).checked).toBe(
      false,
    )
    // The rest of the menu is shown in their place.
    expect(screen.queryByTestId('connectors')).not.toBeInTheDocument()
  })

  it('attaches a connector that is toggled on', async () => {
    const associateMCPServer = jest.fn()
    await renderMenu({ uiHandler: { associateMCPServer } })
    await openConnectors()

    await click(getToggle('Docs'))

    expect(associateMCPServer).toHaveBeenCalledWith('docs', CONVERSATION_UUID)
  })

  it('detaches a connector that is toggled off', async () => {
    const associateMCPServer = jest.fn()
    const disassociateContent = jest.fn()
    await renderMenu({
      uiHandler: { associateMCPServer, disassociateContent },
    })
    await openConnectors()

    await click(getToggle('Issue tracker'))

    expect(disassociateContent).toHaveBeenCalledWith(
      expect.objectContaining({ uuid: 'issues-content' }),
      CONVERSATION_UUID,
    )
    expect(associateMCPServer).not.toHaveBeenCalled()
  })

  it('turns the tools of an attached connector back on', async () => {
    const associateMCPServer = jest.fn()
    const setToolsAttached = jest.fn()
    await renderMenu({
      associatedContent: [{ ...ISSUES_CONTENT, toolsAttached: false }],
      uiHandler: { associateMCPServer },
      conversationHandler: { setToolsAttached },
    })
    await openConnectors()

    expect(
      (getToggle('Issue tracker').querySelector('leo-toggle') as any).checked,
    ).toBe(false)
    await click(getToggle('Issue tracker'))

    expect(setToolsAttached).toHaveBeenCalledWith(
      expect.objectContaining({ uuid: 'issues-content' }),
      true,
    )
    expect(associateMCPServer).not.toHaveBeenCalled()
  })

  it('opens the form for adding a connector', async () => {
    const setConnectorsDialog = jest.fn()
    await renderMenu({ setConnectorsDialog })
    await openConnectors()

    await click(screen.getByTestId('add-connector'))

    expect(setConnectorsDialog).toHaveBeenCalledWith({ view: 'editor' })
  })

  // Adds a stand-in for the composer.
  function addComposer() {
    const composer = document.createElement('div')
    composer.setAttribute('data-editor', '')
    composer.tabIndex = 0
    document.body.appendChild(composer)
    cleanups.push(() => composer.remove())
    return composer
  }

  // Whether focus was given to the connectors' item with `focusKey`.
  function wasFocused(focus: jest.SpyInstance, focusKey: string) {
    return focus.mock.contexts.some(
      (element) =>
        (element as HTMLElement).isConnected
        && (element as HTMLElement).dataset.focusKey === focusKey,
    )
  }

  it('leaves focus in the menu when a connector is detached', async () => {
    // The composer would be focused on the next frame.
    const raf = jest
      .spyOn(window, 'requestAnimationFrame')
      .mockImplementation((callback: FrameRequestCallback) => {
        callback(performance.now())
        return 0
      })
    cleanups.push(() => raf.mockRestore())
    const focusComposer = jest.spyOn(addComposer(), 'focus')
    const disassociateContent = jest.fn()
    await renderMenu({ uiHandler: { disassociateContent } })
    await openConnectors()

    await click(getToggle('Issue tracker'))

    expect(disassociateContent).toHaveBeenCalled()
    expect(focusComposer).not.toHaveBeenCalled()
  })

  it.each([
    ['Enter', 'Enter'],
    [' ', 'Space'],
  ])('toggles a connector once for %j on its switch', async (key, code) => {
    const associateMCPServer = jest.fn()
    await renderMenu({ uiHandler: { associateMCPServer } })
    await openConnectors()
    // Like Leo's menu, which clicks the item that Enter or Space is pressed
    // in.
    const menu = document.querySelector('leo-buttonmenu')!
    menu.addEventListener('keypress', (event: Event) => {
      const { code } = event as KeyboardEvent
      if (code === 'Enter' || code === 'Space') {
        ;(
          event
            .composedPath()
            .find(
              (node) => (node as Element).tagName === 'LEO-MENU-ITEM',
            ) as HTMLElement
        )?.click()
      }
    })

    // The keys reach the switch's host from the button inside it, which
    // toggles the switch and tells the menu that it changed.
    const toggle = getToggle('Docs').querySelector('leo-toggle')!
    const init = { key, code, bubbles: true, composed: true }
    await act(async () => {
      toggle.dispatchEvent(new KeyboardEvent('keydown', init))
      toggle.dispatchEvent(new KeyboardEvent('keypress', init))
      toggle.dispatchEvent(new KeyboardEvent('keyup', init))
      toggle.dispatchEvent(
        Object.assign(new Event('change'), { checked: true }),
      )
    })

    expect(associateMCPServer).toHaveBeenCalledTimes(1)
  })

  it('gives focus back to its item when the connectors change', async () => {
    const observer: { current?: Mojom.MCPHostObserverInterface } = {}
    await renderMenu({ mcpHostObserverRef: observer })
    await openConnectors()
    await act(async () => {
      fireEvent.focusIn(getToggle('Docs'))
    })
    const focus = jest.spyOn(HTMLElement.prototype, 'focus')
    cleanups.push(() => focus.mockRestore())

    // A connector added elsewhere opens the menu afresh.
    await act(async () => {
      observer.current!.onServersChanged([
        ISSUES,
        DOCS,
        { ...DOCS, id: 'wiki', name: 'Wiki', slug: 'wiki' },
      ])
    })

    await waitFor(() => expect(wasFocused(focus, 'connector:docs')).toBe(true))
    expect(screen.getAllByTestId('connector-toggle')).toHaveLength(3)
  })

  it("doesn't take focus from the composer when the connectors change", async () => {
    const observer: { current?: Mojom.MCPHostObserverInterface } = {}
    await renderMenu({ mcpHostObserverRef: observer })
    await openConnectors()
    await act(async () => {
      fireEvent.focusIn(getToggle('Docs'))
    })
    const composer = addComposer()
    composer.focus()
    const focus = jest.spyOn(HTMLElement.prototype, 'focus')
    cleanups.push(() => focus.mockRestore())

    await act(async () => {
      observer.current!.onServersChanged([
        ISSUES,
        DOCS,
        { ...DOCS, id: 'wiki', name: 'Wiki', slug: 'wiki' },
      ])
      await new Promise((resolve) => setTimeout(resolve, 20))
    })

    expect(screen.getAllByTestId('connector-toggle')).toHaveLength(3)
    expect(wasFocused(focus, 'connector:docs')).toBe(false)
    expect(document.activeElement).toBe(composer)
  })

  it('opens on the rest of the menu if it closed while switching', async () => {
    await renderMenu()

    await act(async () => {
      fireEvent.click(screen.getByTestId('connectors'))
      // Closed before the switch to the connectors, which waits for the click
      // to be over.
      document
        .querySelector('leo-buttonmenu')!
        .dispatchEvent(Object.assign(new Event('change'), { isOpen: false }))
      await new Promise((resolve) => setTimeout(resolve, 10))
    })

    expect(screen.getByTestId('connectors')).toBeInTheDocument()
    expect(screen.queryByTestId('connectors-back')).not.toBeInTheDocument()
  })

  it('goes back to the rest of the menu', async () => {
    await renderMenu()
    await openConnectors()

    await click(screen.getByTestId('connectors-back'))

    await waitFor(() =>
      expect(screen.getByTestId('connectors')).toBeInTheDocument(),
    )
    expect(screen.queryByTestId('connector-toggle')).not.toBeInTheDocument()
  })
})
