// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { act, render, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { ContentType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import * as Mojom from '../../../common/mojom'
import { MockContext, MockContextProps } from '../../state/mock_context'
import TabsMenu from './attachments_menu'

// The menu's open state is gated on a transition of `query !== null`. Render
// once without input so any async data (bookmarks, history) can settle, then
// rerender with the desired `inputText` so the transition fires and the menu
// actually opens.
async function renderOpenedMenu(props: Omit<MockContextProps, 'children'>) {
  const result = render(
    <MockContext
      {...props}
      conversationOverrides={undefined}
    >
      <TabsMenu />
    </MockContext>,
  )
  // Flush any pending microtasks (async fetches like bookmarks/history) so
  // the data is ready before the menu transitions to open.
  await act(async () => {
    await Promise.resolve()
  })
  result.rerender(
    <MockContext {...props}>
      <TabsMenu />
    </MockContext>,
  )
  return result
}

describe('TabsMenu', () => {
  it('should render tabs', async () => {
    const { findByText, container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://tes1t.com' },
            id: 1,
          },
          {
            contentId: 2,
            title: 'Test 2',
            url: { url: 'https://test2.com' },
            id: 2,
          },
        ],
      },
    })

    expect(await findByText('Test 1')).toBeInTheDocument()
    expect(await findByText('Test 2')).toBeInTheDocument()
    expect(container.querySelector('img[src*="tes1t.com"]')).toBeInTheDocument()
    expect(container.querySelector('img[src*="test2.com"]')).toBeInTheDocument()
  })

  it('should still show already attached tabs so they can be re-mentioned', async () => {
    // Re-mentioning an attached tab re-runs tool detection on the browser side,
    // re-attaching its tools when it exposes any, so attached tabs must remain
    // selectable in the menu.
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        conversationState: {
          associatedContent: [
            {
              conversationTurnUuid: '1',
              contentId: 1,
              title: 'Test 1',
              url: {
                url: 'https://test1.com',
              },
              contentType: ContentType.PageContent,
              contentUsedPercentage: 0,
              uuid: '1',
              toolsAttached: false,
              mcpServer: undefined,
            },
          ],
        },
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://test1.com' },
            id: 1,
          },
          {
            contentId: 2,
            title: 'Test 2',
            url: { url: 'https://test2.com' },
            id: 2,
          },
        ],
      },
    })

    expect(await findByText('Test 1')).toBeInTheDocument()
    expect(await findByText('Test 2')).toBeInTheDocument()
  })

  it('re-mentioning an already attached tab attaches its tools instead of re-associating', async () => {
    const associateTab = jest.fn()
    const setToolsAttached = jest.fn()
    const attachedContent = {
      conversationTurnUuid: undefined,
      contentId: 1,
      title: 'Test 1',
      url: { url: 'https://test1.com' },
      contentType: ContentType.PageContent,
      contentUsedPercentage: 0,
      uuid: 'content-1',
      toolsAttached: false,
      mcpServer: undefined,
    }
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        conversationState: {
          conversationUuid: '1',
          associatedContent: [attachedContent],
        },
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://test1.com' },
            id: 1,
          },
        ],
      },
      uiHandler: { associateTab },
      conversationHandler: { setToolsAttached },
    })

    const item = await findByText('Test 1')
    await act(() => item.click())

    expect(setToolsAttached).toHaveBeenCalledWith(
      expect.objectContaining({ contentId: 1, uuid: 'content-1' }),
      true,
    )
    expect(associateTab).not.toHaveBeenCalled()
  })

  it("doesn't take a page at a connector's origin for the connector", async () => {
    const setToolsAttached = jest.fn()
    const associateUrlContent = jest.fn()
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      bookmarksService: {
        getBookmarks: () =>
          Promise.resolve({
            bookmarks: [
              {
                id: BigInt(1),
                title: 'Mail home',
                url: { url: 'https://gmail.example.com/' },
              },
            ],
          }),
      },
      initialState: {
        conversationState: {
          conversationUuid: 'conversation-uuid',
          associatedContent: [
            {
              uuid: 'connector-content',
              contentType: ContentType.MCPServer,
              title: 'Gmail',
              contentId: -1,
              url: { url: 'https://gmail.example.com/' },
              contentUsedPercentage: 100,
              conversationTurnUuid: undefined,
              toolsAttached: true,
            },
          ],
        },
      },
      uiHandler: { associateUrlContent },
      conversationHandler: { setToolsAttached },
    })

    const item = await findByText('Mail home')
    await act(async () => {
      item.closest<HTMLElement>('leo-menu-item')!.click()
    })

    expect(associateUrlContent).toHaveBeenCalled()
    expect(setToolsAttached).not.toHaveBeenCalled()
  })

  it('should be open when query starts with @', async () => {
    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://tes1t.com' },
            id: 1,
          },
        ],
      },
    })

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', true)
  })

  it('should be close when @ is removed', async () => {
    render(
      <MockContext conversationOverrides={{ inputText: ['@'] }}>
        <TabsMenu />
      </MockContext>,
    )

    const { container } = render(
      <MockContext conversationOverrides={{ inputText: ['hi'] }}>
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', false)
  })

  it('should filter by text after @', async () => {
    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@2'] },
      initialState: {
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://tes1t.com' },
            id: 1,
          },
          {
            contentId: 2,
            title: 'Test 2',
            url: { url: 'https://test2.com' },
            id: 2,
          },
        ],
      },
    })

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('2')
    })
  })

  it('selecting an element should clear text and attempt to associate with current conversation', async () => {
    const associateTab = jest.fn()
    const tab1 = {
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1,
    }
    const { queryByText, findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        tabs: [
          tab1,
          {
            contentId: 2,
            title: 'Test 2',
            url: { url: 'https://test2.com' },
            id: 2,
          },
        ],
        conversationState: {
          conversationUuid: '1',
        },
      },
      uiHandler: { associateTab },
    })

    // Wait for tabs to render and async data to load
    const item = await findByText('Test 1')
    await act(() => item.click())

    expect(associateTab).toHaveBeenCalledWith(tab1, '1')
    expect(queryByText('@')).not.toBeInTheDocument()
  })

  it('should render bookmarks in the list', async () => {
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      bookmarksService: {
        getBookmarks: () =>
          Promise.resolve({
            bookmarks: [
              {
                id: BigInt(1),
                title: 'Brave Browser',
                url: { url: 'https://brave.com' },
              },
              {
                id: BigInt(2),
                title: 'MDN Web Docs',
                url: { url: 'https://developer.mozilla.org' },
              },
            ],
          }),
      },
    })

    expect(await findByText('Brave Browser')).toBeInTheDocument()
    expect(await findByText('MDN Web Docs')).toBeInTheDocument()
  })

  it('should filter bookmarks by query', async () => {
    const onFetch = jest.fn()

    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@brave'] },
      bookmarksService: {
        getBookmarks: () => {
          onFetch()
          return Promise.resolve({
            bookmarks: [
              {
                id: BigInt(1),
                title: 'Brave Browser',
                url: { url: 'https://brave.com' },
              },
              {
                id: BigInt(2),
                title: 'MDN Web Docs',
                url: { url: 'https://developer.mozilla.org' },
              },
            ],
          })
        },
      },
    })

    await waitFor(() => expect(onFetch).toHaveBeenCalled())

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('Brave')
    })
  })

  it('should render history in the list', async () => {
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      historyService: {
        getHistory: () =>
          Promise.resolve({
            history: [
              {
                id: BigInt(1),
                title: 'Brave Search',
                url: { url: 'https://search.brave.com' },
              },
              {
                id: BigInt(2),
                title: 'GitHub',
                url: { url: 'https://github.com' },
              },
            ],
          }),
      },
    })

    expect(await findByText('Brave Search')).toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })

  it('should filter history by query', async () => {
    const onFetchHistory = jest.fn()
    const historyService = {
      getHistory: () => {
        onFetchHistory()
        return Promise.resolve({
          history: [
            {
              id: BigInt(1),
              title: 'Brave Search',
              url: { url: 'https://search.brave.com' },
            },
            {
              id: BigInt(2),
              title: 'GitHub',
              url: { url: 'https://github.com' },
            },
          ],
        })
      },
    }

    // Open the menu with a bare '@' so the history fetch can settle, then
    // rerender with the actual query so the filter applies against loaded data.
    const { container, rerender } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      historyService,
    })
    await act(async () => {
      await Promise.resolve()
    })
    rerender(
      <MockContext
        conversationOverrides={{ inputText: ['@search'] }}
        historyService={historyService}
      >
        <TabsMenu />
      </MockContext>,
    )

    // Wait for history to be queried with the new filter.
    await waitFor(() => expect(onFetchHistory).toHaveBeenCalled())

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('Search')
    })
  })

  it('should pass query to getHistory when query length >= 2', async () => {
    const getHistory = jest.fn(() =>
      Promise.resolve({
        history: [
          {
            id: BigInt(1),
            title: 'Test History',
            url: { url: 'https://test.com' },
          },
        ] satisfies Mojom.HistoryEntry[],
      }),
    )

    render(
      <MockContext
        conversationOverrides={{ inputText: ['@ab'] }}
        historyService={{ getHistory }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getHistory).toHaveBeenCalledWith('ab', null)
  })

  it('should filter out already attached history items', async () => {
    const { findByText, queryByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        conversationState: {
          associatedContent: [
            {
              conversationTurnUuid: undefined,
              contentId: 1,
              title: 'Brave Search',
              url: {
                url: 'https://search.brave.com',
              },
              contentType: ContentType.PageContent,
              contentUsedPercentage: 0,
              uuid: '1',
              toolsAttached: false,
              mcpServer: undefined,
            },
          ],
        },
      },
      historyService: {
        getHistory: () =>
          Promise.resolve({
            history: [
              {
                id: BigInt(1),
                title: 'Brave Search',
                url: { url: 'https://search.brave.com' },
              },
              {
                id: BigInt(2),
                title: 'GitHub',
                url: { url: 'https://github.com' },
              },
            ],
          }),
      },
    })

    expect(await findByText('GitHub')).toBeInTheDocument()
    expect(queryByText('Brave Search')).not.toBeInTheDocument()
  })

  describe('attachment insertion', () => {
    it('should insert attachment in place of @ trigger when selecting', async () => {
      const associateTab = jest.fn()

      // Create a simple editor element
      const editor = document.createElement('div')
      editor.setAttribute('data-editor', 'true')
      editor.contentEditable = 'true'
      editor.textContent = '@test'
      document.body.appendChild(editor)

      const tab1 = {
        contentId: 1,
        title: 'Test Tab',
        url: { url: 'https://test.com' },
        id: 1,
      }

      const { container } = await renderOpenedMenu({
        conversationOverrides: { inputText: ['@test'] },
        initialState: {
          conversationState: { conversationUuid: '1' },
          tabs: [tab1],
        },
        uiHandler: { associateTab },
      })

      // Wait for the menu item to render. The title is split across nested
      // spans by MatchedText highlighting, so query directly.
      const menuItem = await waitFor(() => {
        const item = container.querySelector('leo-menu-item')
        expect(item).toBeTruthy()
        return item!
      })

      // Place the selection at the end of '@test' immediately before clicking.
      // selectAttachment requires the active selection to live inside the
      // editor (EditorAPI#canEdit), and rendering the menu can perturb it.
      const range = document.createRange()
      const textNode = editor.firstChild!
      range.setStart(textNode, 5)
      range.setEnd(textNode, 5)
      const selection = window.getSelection()!
      selection.removeAllRanges()
      selection.addRange(range)

      menuItem.click()

      // Check that attachment was associated
      expect(associateTab).toHaveBeenCalledWith(tab1, '1')

      // Check that editor content was replaced with attachment
      const attachmentNode = editor.querySelector('[data-type="attachment"]')
      expect(attachmentNode).toBeTruthy()
      expect(attachmentNode?.textContent).toBe('Test Tab')
      expect(attachmentNode?.getAttribute('data-id')).toBe('1')

      // Check that @ trigger is gone
      expect(editor.textContent?.includes('@test')).toBe(false)

      // Cleanup
      document.body.removeChild(editor)
    })

    it('should insert attachment after existing text', async () => {
      const associateTab = jest.fn()

      // Create editor with text before the trigger
      const editor = document.createElement('div')
      editor.setAttribute('data-editor', 'true')
      editor.contentEditable = 'true'
      editor.textContent = 'Hello @my'
      document.body.appendChild(editor)

      const tab1 = {
        contentId: 1,
        title: 'My Tab',
        url: { url: 'https://example.com' },
        id: 1,
      }

      const { container } = await renderOpenedMenu({
        conversationOverrides: { inputText: ['Hello @my'] },
        initialState: {
          conversationState: { conversationUuid: '1' },
          tabs: [tab1],
        },
        uiHandler: { associateTab },
      })

      // Wait for the menu item to render. The title is split across nested
      // spans by MatchedText highlighting, so query directly.
      const menuItem = await waitFor(() => {
        const item = container.querySelector('leo-menu-item')
        expect(item).toBeTruthy()
        return item!
      })

      // Place the selection at the end of 'Hello @my' immediately before
      // clicking. selectAttachment requires the active selection to live
      // inside the editor (EditorAPI#canEdit), and rendering the menu can
      // perturb it.
      const range = document.createRange()
      const textNode = editor.firstChild!
      range.setStart(textNode, 9)
      range.setEnd(textNode, 9)
      const selection = window.getSelection()!
      selection.removeAllRanges()
      selection.addRange(range)

      menuItem.click()

      // Check that 'Hello ' remains and attachment was inserted
      expect(editor.textContent?.startsWith('Hello ')).toBe(true)

      const attachmentNode = editor.querySelector('[data-type="attachment"]')
      expect(attachmentNode).toBeTruthy()
      expect(attachmentNode?.textContent).toBe('My Tab')

      // Check that @my trigger is gone
      expect(editor.textContent?.includes('@my')).toBe(false)

      // Cleanup
      document.body.removeChild(editor)
    })
  })
})

describe('TabsMenu connectors', () => {
  const TOOLS_MODEL = {
    key: 'tools-model',
    displayName: 'Tools model',
    isSuggestedModel: true,
    isNearModel: false,
    visionSupport: false,
    supportsTools: true,
    supportedCapabilities: [],
    options: {
      customModelOptions: undefined,
      leoModelOptions: undefined,
    },
  } as unknown as Mojom.Model

  const server = (
    id: string,
    name: string,
    overrides: Partial<Mojom.MCPServer> = {},
  ): Mojom.MCPServer => ({
    id,
    name,
    endpoint: { url: `https://${id}.example.com/secret/mcp` },
    enabled: true,
    autoAttach: false,
    slug: id,
    hasToken: false,
    tokenUnreadable: false,
    ...overrides,
  })

  const GMAIL = server('gmail', 'Gmail')
  const GROUP_EMAIL = server('group', 'Group email')
  const DISABLED = server('disabled', 'Gmail archive', { enabled: false })
  const UNREADABLE = server('unreadable', 'Gmail backup', {
    hasToken: true,
    tokenUnreadable: true,
  })

  const GMAIL_CONTENT: Mojom.AssociatedContent = {
    uuid: 'gmail-content',
    contentType: ContentType.MCPServer,
    title: 'Gmail',
    contentId: -1,
    url: { url: 'https://gmail.example.com/' },
    contentUsedPercentage: 100,
    conversationTurnUuid: undefined,
    toolsAttached: false,
    mcpServer: {
      serverId: 'gmail',
      status: Mojom.MCPServerStatus.kAvailable,
      missedLatestAnswer: false,
    },
  }

  beforeEach(() => {
    clearAllDataForTesting()
  })

  function renderWithConnectors(
    inputText: string,
    options: {
      servers?: Mojom.MCPServer[]
      tabs?: Mojom.TabData[]
      associatedContent?: Mojom.AssociatedContent[]
      supportsTools?: boolean
      isMCPEnabled?: boolean
      conversationUuid?: string
      getServers?: jest.Mock
      uiHandler?: MockContextProps['uiHandler']
      conversationHandler?: MockContextProps['conversationHandler']
    } = {},
  ) {
    const servers = options.servers ?? [GMAIL, DISABLED, UNREADABLE]
    return renderOpenedMenu({
      conversationOverrides: { inputText: [inputText] },
      aiChatOverrides: { isMCPEnabled: options.isMCPEnabled ?? true },
      mcpHost: {
        getServers:
          options.getServers?.mockResolvedValue({ servers })
          ?? (() => Promise.resolve({ servers })),
      },
      initialState: {
        tabs: options.tabs ?? [],
        conversationState: {
          conversationUuid: options.conversationUuid ?? 'conversation-uuid',
          allModels: [
            { ...TOOLS_MODEL, supportsTools: options.supportsTools ?? true },
          ],
          currentModelKey: TOOLS_MODEL.key,
          associatedContent: options.associatedContent ?? [],
        },
      },
      uiHandler: options.uiHandler,
      conversationHandler: options.conversationHandler,
    })
  }

  const mentionedNames = (container: HTMLElement) =>
    Array.from(container.querySelectorAll('leo-menu-item')).map((item) => ({
      name: item.textContent,
      isConnector: item.getAttribute('data-testid') === 'connector-mention',
    }))

  it('offers the connectors that are turned on and usable', async () => {
    const { findByText, container } = await renderWithConnectors('@')

    expect(await findByText('Gmail')).toBeInTheDocument()
    expect(mentionedNames(container)).toEqual([
      { name: 'Gmail', isConnector: true },
    ])
  })

  it("offers no connectors to a model that can't use tools", async () => {
    const getServers = jest.fn()
    const { findByText, container } = await renderWithConnectors('@', {
      getServers,
      supportsTools: false,
      tabs: [
        { id: 1, contentId: 1, title: 'Tab', url: { url: 'https://a.com' } },
      ],
    })

    expect(await findByText('Tab')).toBeInTheDocument()
    expect(
      container.querySelector('[data-testid="connector-mention"]'),
    ).toBeNull()
    expect(getServers).not.toHaveBeenCalled()
  })

  // A mention of a connector that can't be attached would mislead the model.
  it('offers no connectors before there is a conversation', async () => {
    const getServers = jest.fn()
    const { findByText, container } = await renderWithConnectors('@', {
      getServers,
      conversationUuid: '',
      tabs: [
        { id: 1, contentId: 1, title: 'Tab', url: { url: 'https://a.com' } },
      ],
    })

    expect(await findByText('Tab')).toBeInTheDocument()
    expect(
      container.querySelector('[data-testid="connector-mention"]'),
    ).toBeNull()
    expect(getServers).not.toHaveBeenCalled()
  })

  it("offers no connectors where they aren't enabled", async () => {
    const getServers = jest.fn()
    const { findByText, container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      aiChatOverrides: { isMCPEnabled: false },
      mcpHost: { getServers },
      initialState: {
        tabs: [
          { id: 1, contentId: 1, title: 'Tab', url: { url: 'https://a.com' } },
        ],
        conversationState: {
          allModels: [TOOLS_MODEL],
          currentModelKey: TOOLS_MODEL.key,
        },
      },
    })

    expect(await findByText('Tab')).toBeInTheDocument()
    expect(
      container.querySelector('[data-testid="connector-mention"]'),
    ).toBeNull()
    expect(getServers).not.toHaveBeenCalled()
  })

  it('ranks connectors among the content they match', async () => {
    const tab = (id: number, title: string): Mojom.TabData => ({
      id,
      contentId: id,
      title,
      url: { url: `https://tab${id}.example.com` },
    })
    const { container } = await renderWithConnectors('@gmail', {
      servers: [GROUP_EMAIL, GMAIL],
      tabs: [tab(1, 'My Gmail'), tab(2, 'Gmail')],
    })

    await waitFor(() =>
      expect(mentionedNames(container)).toEqual([
        { name: 'Gmail', isConnector: true },
        { name: 'Gmail', isConnector: false },
        { name: 'My Gmail', isConnector: false },
        { name: 'Group email', isConnector: true },
      ]),
    )
  })

  it('attaches a connector that is mentioned', async () => {
    const associateMCPServer = jest.fn()
    const setToolsAttached = jest.fn()
    const editor = document.createElement('div')
    editor.setAttribute('data-editor', 'true')
    editor.contentEditable = 'true'
    editor.textContent = '@gm'
    document.body.appendChild(editor)

    const { container } = await renderWithConnectors('@gm', {
      uiHandler: { associateMCPServer },
      conversationHandler: { setToolsAttached },
    })
    // Its name is split up by the highlighting of what matched.
    const item = await waitFor(() => {
      const connector = container.querySelector<HTMLElement>(
        '[data-testid="connector-mention"]',
      )
      expect(connector).toHaveTextContent('Gmail')
      return connector!
    })

    const selection = window.getSelection()!
    selection.removeAllRanges()
    const range = document.createRange()
    range.setStart(editor.firstChild!, 3)
    range.setEnd(editor.firstChild!, 3)
    selection.addRange(range)
    await act(async () => {
      item.click()
    })

    expect(associateMCPServer).toHaveBeenCalledWith(
      'gmail',
      'conversation-uuid',
    )
    expect(setToolsAttached).not.toHaveBeenCalled()

    // Mentioned like a tab, by its name, with only its origin as its URL.
    const mention = editor.querySelector('[data-type="attachment"]')
    expect(mention?.textContent).toBe('Gmail')
    expect(mention?.getAttribute('data-id')).toBe('gmail')
    expect(mention?.getAttribute('data-url')).toBe('https://gmail.example.com')
    document.body.removeChild(editor)
  })

  it('turns the tools of an attached connector back on', async () => {
    const associateMCPServer = jest.fn()
    const setToolsAttached = jest.fn()
    const { findByText } = await renderWithConnectors('@', {
      associatedContent: [GMAIL_CONTENT],
      uiHandler: { associateMCPServer },
      conversationHandler: { setToolsAttached },
    })

    const item = await findByText('Gmail')
    await act(async () => {
      item.closest<HTMLElement>('leo-menu-item')!.click()
    })

    expect(setToolsAttached).toHaveBeenCalledWith(
      expect.objectContaining({ uuid: 'gmail-content' }),
      true,
    )
    expect(associateMCPServer).not.toHaveBeenCalled()
  })
})
