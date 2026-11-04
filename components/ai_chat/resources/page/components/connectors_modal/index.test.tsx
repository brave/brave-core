// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, waitFor, fireEvent } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../../common/mojom'
import { MockContext } from '../../state/mock_context'
import { ConnectorsDialog, useAIChat } from '../../state/ai_chat_context'
import { MCPHostInterface } from '../../api/ai_chat_api'
import ConnectorsModal from './index'

const ISSUES: Mojom.MCPServer = {
  id: 'issues',
  name: 'Issue tracker',
  endpoint: { url: 'https://mcp.example.com/secret-key/mcp' },
  enabled: true,
  autoAttach: true,
  slug: 'issuetracker',
  hasToken: true,
  tokenUnreadable: false,
}

const CALENDAR: Mojom.MCPServer = {
  id: 'calendar',
  name: 'Calendar',
  endpoint: { url: 'https://calendar.example.net/mcp' },
  enabled: true,
  autoAttach: false,
  slug: 'calendar',
  hasToken: true,
  tokenUnreadable: true,
}

const NOTES: Mojom.MCPServer = {
  id: 'notes',
  name: 'Local notes',
  endpoint: { url: 'http://localhost:8080/mcp' },
  enabled: false,
  autoAttach: false,
  slug: 'localnotes',
  hasToken: false,
  tokenUnreadable: false,
}

const TOOLS: Mojom.MCPServerTool[] = [
  { name: 'search', title: undefined, description: undefined },
  { name: 'create', title: undefined, description: undefined },
]

type Observer = { current?: Mojom.MCPHostObserverInterface }

// Opens the dialog the way the page does, through the context.
function Opener(props: { dialog: ConnectorsDialog }) {
  const aiChat = useAIChat()
  React.useEffect(() => {
    aiChat.setConnectorsDialog(props.dialog)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])
  return aiChat.connectorsDialog ? <ConnectorsModal /> : <div>closed</div>
}

interface RenderOptions {
  dialog?: ConnectorsDialog
  servers?: Mojom.MCPServer[]
  mcpHost?: Partial<MCPHostInterface>
  observer?: Observer
}

async function renderModal(options: RenderOptions = {}) {
  const getServerTools = jest.fn((id: string) => Promise.resolve(TOOLS))
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(
      <MockContext
        uiHandler={{
          getPluralString: (key, count) =>
            Promise.resolve({ pluralString: `${key}:${count}` }),
        }}
        mcpHost={{
          getServers: () =>
            Promise.resolve({ servers: options.servers ?? [ISSUES] }),
          getServerTools,
          ...options.mcpHost,
        }}
        mcpHostObserverRef={options.observer}
      >
        <Opener dialog={options.dialog ?? { view: 'list' }} />
      </MockContext>,
    )
  })
  return { ...result!, getServerTools }
}

// The host's observer message arrives on its own, after the call that caused
// it returns.
function notifyLater(observer: Observer, servers: Mojom.MCPServer[]) {
  setTimeout(() => observer.current!.onServersChanged(servers))
}

function deferred<T>() {
  let settle!: { resolve: (value: T) => void; reject: (error: unknown) => void }
  const promise = new Promise<T>((resolve, reject) => {
    settle = { resolve, reject }
  })
  return { promise, ...settle }
}

function inputEvent(value: string) {
  return Object.assign(new Event('input', { bubbles: true }), { value })
}

function changeEvent(checked: boolean) {
  return Object.assign(new Event('change'), { checked })
}

function enterEvent() {
  return Object.assign(new Event('keydown'), {
    innerEvent: new KeyboardEvent('keydown', { key: 'Enter' }),
  })
}

async function click(element: HTMLElement) {
  await act(async () => {
    fireEvent.click(element)
  })
}

async function fill(testId: string, value: string) {
  await act(async () => {
    screen.getByTestId(testId).dispatchEvent(inputEvent(value))
  })
}

async function save() {
  await click(screen.getByText('CHAT_UI_SAVE_BUTTON_LABEL'))
}

describe('ConnectorsModal', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  describe('list', () => {
    it('shows each connector by name and origin, never its path', async () => {
      await renderModal()

      expect(await screen.findByText('Issue tracker')).toBeInTheDocument()
      expect(screen.getByText('https://mcp.example.com')).toBeInTheDocument()
      expect(screen.queryByText(/secret-key/)).not.toBeInTheDocument()
      expect(
        screen.getByRole('group', { name: 'Issue tracker' }),
      ).toBeInTheDocument()
    })

    it('says so when there is no host to ask', async () => {
      await renderModal({
        mcpHost: {
          getServers: () =>
            Promise.reject(new Error('The pipe has already been closed.')),
        },
      })

      expect(
        await screen.findByText('CHAT_UI_ERROR_INTERNAL'),
      ).toBeInTheDocument()
      expect(
        screen.queryByText('CHAT_UI_CONNECTORS_EMPTY'),
      ).not.toBeInTheDocument()
    })

    it('shows an empty state when there are no connectors', async () => {
      await renderModal({ servers: [] })

      expect(
        await screen.findByText('CHAT_UI_CONNECTORS_EMPTY'),
      ).toBeInTheDocument()
    })

    it('lists tools only of the connectors it may connect to', async () => {
      const { getServerTools } = await renderModal({
        servers: [ISSUES, CALENDAR, NOTES],
      })

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_TOOL_COUNT:2'),
      ).toBeInTheDocument()
      expect(
        screen.getByText('CHAT_UI_CONNECTOR_ERROR_TOKEN_UNREADABLE'),
      ).toBeInTheDocument()
      // Neither a connector that's turned off, nor one whose token can't be
      // read, is connected to.
      expect(getServerTools).toHaveBeenCalledTimes(1)
      expect(getServerTools).toHaveBeenCalledWith(ISSUES.id)
    })

    it('shows why a connector could not be connected to', async () => {
      await renderModal({
        mcpHost: {
          getServerTools: () =>
            Promise.reject(Mojom.MCPServerError.kConnectionFailed),
        },
      })

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_ERROR_CONNECTION_FAILED'),
      ).toBeInTheDocument()
    })

    it("doesn't reconnect to a failed connector when the list is shown again", async () => {
      const getServerTools = jest.fn(() =>
        Promise.reject(Mojom.MCPServerError.kConnectionFailed),
      )
      await renderModal({ mcpHost: { getServerTools } })
      await screen.findByText('CHAT_UI_CONNECTOR_ERROR_CONNECTION_FAILED')

      await click(screen.getByTitle('CHAT_UI_EDIT_BUTTON_LABEL'))
      await click(screen.getByText('CHAT_UI_CANCEL_BUTTON_LABEL'))

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_ERROR_CONNECTION_FAILED'),
      ).toBeInTheDocument()
      expect(getServerTools).toHaveBeenCalledTimes(1)
    })

    it('turns a connector off, keeping its other settings', async () => {
      const updateServer = jest.fn(() => Promise.resolve(ISSUES))
      await renderModal({ mcpHost: { updateServer } })
      await screen.findByText('Issue tracker')

      await act(async () => {
        screen
          .getByTestId('connector-enabled')
          .dispatchEvent(changeEvent(false))
      })

      expect(updateServer).toHaveBeenCalledWith(
        ISSUES.id,
        {
          name: ISSUES.name,
          endpoint: ISSUES.endpoint,
          enabled: false,
          autoAttach: true,
        },
        null,
      )
      // Shown as off until the host reports the change.
      expect(screen.getByTestId('connector-enabled')).toHaveProperty(
        'checked',
        false,
      )
    })

    it('flips a toggle back, and says why, when a change fails', async () => {
      const update = deferred<Mojom.MCPServer>()
      await renderModal({ mcpHost: { updateServer: () => update.promise } })
      await screen.findByText('Issue tracker')
      const toggle = screen.getByTestId('connector-auto-attach')

      await act(async () => {
        toggle.dispatchEvent(changeEvent(false))
      })
      expect(toggle).toHaveProperty('checked', false)

      await act(async () => {
        update.reject(Mojom.MCPServerError.kUnsupportedSettingsVersion)
      })
      expect(toggle).toHaveProperty('checked', true)
      expect(
        screen.getByText(
          'CHAT_UI_CONNECTOR_ERROR_UNSUPPORTED_SETTINGS_VERSION',
        ),
      ).toBeInTheDocument()
    })

    it("can't add a connector that's turned off to new chats", async () => {
      await renderModal({ servers: [NOTES] })
      await screen.findByText('Local notes')

      expect(screen.getByTestId('connector-auto-attach')).toHaveProperty(
        'disabled',
        true,
      )
    })

    it('follows changes the host reports', async () => {
      const observer: Observer = {}
      const { getServerTools } = await renderModal({
        servers: [ISSUES, { ...NOTES, enabled: true }],
        observer,
      })
      await waitFor(() => expect(getServerTools).toHaveBeenCalledTimes(2))

      await act(async () => {
        observer.current!.onServersChanged([
          // Only a change to how it's connected to lists its tools again.
          { ...ISSUES, autoAttach: false },
          {
            ...NOTES,
            enabled: true,
            endpoint: { url: 'http://localhost:9090/mcp' },
          },
          { ...CALENDAR, id: 'added', name: 'Added', tokenUnreadable: false },
        ])
      })

      // The cache tells React of changes in a later task.
      expect(
        await screen.findByText('http://localhost:9090'),
      ).toBeInTheDocument()
      expect(screen.getByText('Added')).toBeInTheDocument()
      await waitFor(() => expect(getServerTools).toHaveBeenCalledTimes(4))
      const ids = getServerTools.mock.calls.map(([id]) => id)
      expect(ids.filter((id) => id === ISSUES.id)).toHaveLength(1)
      expect(ids.filter((id) => id === NOTES.id)).toHaveLength(2)
      expect(ids).toContain('added')
    })

    it('removes a connector only once the user confirms', async () => {
      const removeServer = jest.fn(() => Promise.resolve({ error: null }))
      await renderModal({ mcpHost: { removeServer } })
      await screen.findByText('Issue tracker')

      await click(screen.getByTitle('CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL'))
      expect(
        screen.getByText('CHAT_UI_CONNECTOR_REMOVE_TITLE'),
      ).toBeInTheDocument()
      expect(removeServer).not.toHaveBeenCalled()

      await click(screen.getByText('CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL'))
      expect(removeServer).toHaveBeenCalledWith(ISSUES.id)
    })

    it('says why a connector could not be removed', async () => {
      await renderModal({
        mcpHost: {
          removeServer: () =>
            Promise.resolve({
              error: Mojom.MCPServerError.kUnsupportedSettingsVersion,
            }),
        },
      })
      await screen.findByText('Issue tracker')

      await click(screen.getByTitle('CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL'))
      await click(screen.getByText('CHAT_UI_CONNECTOR_REMOVE_BUTTON_LABEL'))

      expect(
        await screen.findByText(
          'CHAT_UI_CONNECTOR_ERROR_UNSUPPORTED_SETTINGS_VERSION',
        ),
      ).toBeInTheDocument()
      expect(screen.getByText('Issue tracker')).toBeInTheDocument()
    })

    it('opens the editor and goes back to the list', async () => {
      await renderModal()
      await screen.findByText('Issue tracker')

      await click(screen.getByTitle('CHAT_UI_EDIT_BUTTON_LABEL'))
      expect(
        screen.getByText('CHAT_UI_CONNECTOR_EDIT_TITLE'),
      ).toBeInTheDocument()

      await click(screen.getByText('CHAT_UI_CANCEL_BUTTON_LABEL'))
      expect(screen.getByText('CHAT_UI_CONNECTORS_TITLE')).toBeInTheDocument()
    })

    it('goes back to the list when the connector being edited is removed', async () => {
      const observer: Observer = {}
      await renderModal({ observer })
      await screen.findByText('Issue tracker')
      await click(screen.getByTitle('CHAT_UI_EDIT_BUTTON_LABEL'))

      await act(async () => {
        observer.current!.onServersChanged([])
      })

      expect(
        await screen.findByText('CHAT_UI_CONNECTORS_TITLE'),
      ).toBeInTheDocument()
      expect(screen.getByText('CHAT_UI_CONNECTORS_EMPTY')).toBeInTheDocument()
    })
  })

  describe('editor', () => {
    it('adds a connector, turned on, with the token given', async () => {
      const addServer = jest.fn((params: Mojom.MCPServerParams) =>
        Promise.resolve({ ...ISSUES, ...params }),
      )
      await renderModal({ dialog: { view: 'editor' }, mcpHost: { addServer } })

      await fill('connector-name', '  Issue tracker ')
      await fill('connector-url', 'https://mcp.example.com/mcp')
      await fill('connector-token', 'secret')
      await save()

      expect(addServer).toHaveBeenCalledWith(
        {
          name: 'Issue tracker',
          endpoint: { url: 'https://mcp.example.com/mcp' },
          enabled: true,
          autoAttach: false,
        },
        'secret',
      )
      // Opened on its own, so it closes rather than going to the list.
      expect(await screen.findByText('closed')).toBeInTheDocument()
    })

    it('saves once, however often Enter is pressed', async () => {
      const add = deferred<Mojom.MCPServer>()
      const addServer = jest.fn(() => add.promise)
      await renderModal({ dialog: { view: 'editor' }, mcpHost: { addServer } })

      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'https://mcp.example.com/mcp')
      await act(async () => {
        const input = screen.getByTestId('connector-url')
        input.dispatchEvent(enterEvent())
        input.dispatchEvent(enterEvent())
      })

      expect(addServer).toHaveBeenCalledTimes(1)
    })

    it('stays closed when a save finishes after the dialog closes', async () => {
      const add = deferred<Mojom.MCPServer>()
      await renderModal({ mcpHost: { addServer: () => add.promise } })
      await screen.findByText('Issue tracker')
      await click(screen.getByText('CHAT_UI_CONNECTORS_ADD_BUTTON_LABEL'))
      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'https://mcp.example.com/mcp')
      await save()

      await act(async () => {
        document.querySelector('leo-dialog')!.dispatchEvent(new Event('close'))
      })
      expect(screen.getByText('closed')).toBeInTheDocument()

      await act(async () => {
        add.resolve(ISSUES)
      })
      expect(screen.getByText('closed')).toBeInTheDocument()
      expect(
        screen.queryByText('CHAT_UI_CONNECTORS_TITLE'),
      ).not.toBeInTheDocument()
    })

    it("can't be left while a save is in progress", async () => {
      const update = deferred<Mojom.MCPServer>()
      await renderModal({ mcpHost: { updateServer: () => update.promise } })
      await screen.findByText('Issue tracker')
      await click(screen.getByTitle('CHAT_UI_EDIT_BUTTON_LABEL'))
      const dialog = document.querySelector('leo-dialog')!
      expect(dialog).toHaveProperty('showBack', true)

      await save()

      expect(screen.getByText('CHAT_UI_CANCEL_BUTTON_LABEL')).toHaveProperty(
        'isDisabled',
        true,
      )
      expect(dialog).toHaveProperty('showClose', false)
      expect(dialog).toHaveProperty('showBack', false)
      expect(dialog).toHaveProperty('escapeCloses', false)

      await act(async () => {
        update.resolve(ISSUES)
      })
      expect(
        await screen.findByText('CHAT_UI_CONNECTORS_TITLE'),
      ).toBeInTheDocument()
      expect(dialog).toHaveProperty('showClose', true)
    })

    it('saves again after a save fails', async () => {
      const addServer = jest
        .fn()
        .mockRejectedValueOnce(Mojom.MCPServerError.kTokenEncryptionFailed)
        .mockResolvedValueOnce(ISSUES)
      await renderModal({ dialog: { view: 'editor' }, mcpHost: { addServer } })
      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'https://mcp.example.com/mcp')

      await save()
      expect(
        await screen.findByText(
          'CHAT_UI_CONNECTOR_ERROR_TOKEN_ENCRYPTION_FAILED',
        ),
      ).toBeInTheDocument()
      await save()

      expect(addServer).toHaveBeenCalledTimes(2)
      expect(await screen.findByText('closed')).toBeInTheDocument()
    })

    it("doesn't send a URL that can't be parsed", async () => {
      const addServer = jest.fn()
      await renderModal({ dialog: { view: 'editor' }, mcpHost: { addServer } })

      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'mcp.example.com')
      await save()

      expect(addServer).not.toHaveBeenCalled()
      expect(
        screen.getByText('CHAT_UI_CONNECTOR_ERROR_INVALID_URL'),
      ).toBeInTheDocument()
    })

    it('shows an error from the host at the field it is about', async () => {
      await renderModal({
        dialog: { view: 'editor' },
        mcpHost: {
          addServer: () => Promise.reject(Mojom.MCPServerError.kInsecureScheme),
        },
      })

      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'http://mcp.example.com/mcp')
      await save()

      const error = await screen.findByText(
        'CHAT_UI_CONNECTOR_ERROR_INSECURE_SCHEME',
      )
      expect(error).toHaveAttribute('slot', 'errors')
      expect(screen.getByTestId('connector-url')).toContainElement(error)

      // Editing the field clears its error.
      await fill('connector-url', 'https://mcp.example.com/mcp')
      expect(
        screen.queryByText('CHAT_UI_CONNECTOR_ERROR_INSECURE_SCHEME'),
      ).not.toBeInTheDocument()
    })

    it('shows other errors from the host above the form', async () => {
      await renderModal({
        dialog: { view: 'editor' },
        mcpHost: {
          addServer: () => Promise.reject(Mojom.MCPServerError.kTooManyServers),
        },
      })

      await fill('connector-name', 'Issue tracker')
      await fill('connector-url', 'https://mcp.example.com/mcp')
      await save()

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_ERROR_TOO_MANY_SERVERS'),
      ).toBeInTheDocument()
      expect(screen.queryByText('closed')).not.toBeInTheDocument()
    })

    it('never shows a saved token, and keeps it unless replaced', async () => {
      const updateServer = jest.fn(() => Promise.resolve(ISSUES))
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      expect(screen.queryByTestId('connector-token')).not.toBeInTheDocument()
      await fill('connector-name', 'Issues')
      await save()

      expect(updateServer).toHaveBeenCalledWith(
        ISSUES.id,
        {
          name: 'Issues',
          endpoint: ISSUES.endpoint,
          enabled: true,
          autoAttach: true,
        },
        null,
      )
    })

    it('replaces a saved token', async () => {
      const updateServer = jest.fn(() => Promise.resolve(ISSUES))
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REPLACE_BUTTON_LABEL'),
      )
      await fill('connector-token', 'new-secret')
      await save()

      expect(updateServer).toHaveBeenCalledWith(
        ISSUES.id,
        expect.anything(),
        'new-secret',
      )
    })

    it('removes a saved token on Save, after the update', async () => {
      const observer: Observer = {}
      const calls: string[] = []
      const updateServer = jest.fn(() => {
        calls.push('update')
        return Promise.resolve(ISSUES)
      })
      const clearServerToken = jest.fn(() => {
        calls.push('clear')
        const server = { ...ISSUES, hasToken: false }
        notifyLater(observer, [server])
        return Promise.resolve(server)
      })
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer, clearServerToken },
        observer,
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      const focus = jest.spyOn(HTMLElement.prototype, 'focus')
      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL'),
      )
      expect(clearServerToken).not.toHaveBeenCalled()
      const tokenInput = screen.getByTestId('connector-token')
      expect(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVED_ON_SAVE'),
      ).toBeInTheDocument()
      await waitFor(() => expect(focus.mock.instances).toContain(tokenInput))
      focus.mockRestore()

      await save()

      await waitFor(() => expect(calls).toEqual(['update', 'clear']))
      expect(updateServer).toHaveBeenCalledWith(
        ISSUES.id,
        expect.anything(),
        null,
      )
      expect(clearServerToken).toHaveBeenCalledWith(ISSUES.id)
      expect(await screen.findByText('closed')).toBeInTheDocument()
    })

    it('keeps a token marked for removal when the edit is cancelled', async () => {
      const updateServer = jest.fn(() => Promise.resolve(ISSUES))
      const clearServerToken = jest.fn(() => Promise.resolve(ISSUES))
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer, clearServerToken },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL'),
      )
      await click(screen.getByText('CHAT_UI_CANCEL_BUTTON_LABEL'))

      expect(screen.getByText('closed')).toBeInTheDocument()
      expect(clearServerToken).not.toHaveBeenCalled()
      expect(updateServer).not.toHaveBeenCalled()
    })

    it('replaces a token marked for removal with a new one', async () => {
      const updateServer = jest.fn(() => Promise.resolve(ISSUES))
      const clearServerToken = jest.fn(() => Promise.resolve(ISSUES))
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer, clearServerToken },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL'),
      )
      await fill('connector-token', 'new-secret')
      await save()

      expect(updateServer).toHaveBeenCalledWith(
        ISSUES.id,
        expect.anything(),
        'new-secret',
      )
      expect(clearServerToken).not.toHaveBeenCalled()
    })

    it('warns at the URL that moving to another site drops the token', async () => {
      await renderModal({ dialog: { view: 'editor', serverId: ISSUES.id } })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await fill('connector-url', 'https://mcp.example.com/other')
      expect(
        screen.queryByText('CHAT_UI_CONNECTOR_TOKEN_ORIGIN_CHANGED'),
      ).not.toBeInTheDocument()

      await fill('connector-url', 'https://elsewhere.example.com/mcp')
      const warning = screen.getByText('CHAT_UI_CONNECTOR_TOKEN_ORIGIN_CHANGED')
      // Not hidden when Authentication is collapsed.
      expect(warning.closest('leo-collapse')).toBeNull()
      expect(warning.parentElement).toBe(
        screen.getByTestId('connector-url').parentElement,
      )
    })

    it("doesn't clear a token that moving to another site drops", async () => {
      const moved = {
        ...ISSUES,
        endpoint: { url: 'https://elsewhere.example.com/mcp' },
        hasToken: false,
      }
      const updateServer = jest.fn(() => Promise.resolve(moved))
      const clearServerToken = jest.fn(() => Promise.resolve(moved))
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { updateServer, clearServerToken },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await fill('connector-url', moved.endpoint.url)
      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL'),
      )
      await save()

      expect(await screen.findByText('closed')).toBeInTheDocument()
      expect(updateServer).toHaveBeenCalledTimes(1)
      expect(clearServerToken).not.toHaveBeenCalled()
    })

    it('asks for an unreadable token again, or lets it be removed', async () => {
      const updateServer = jest.fn(() => Promise.resolve(CALENDAR))
      const clearServerToken = jest.fn(() => Promise.resolve(CALENDAR))
      await renderModal({
        dialog: { view: 'editor', serverId: CALENDAR.id },
        servers: [CALENDAR],
        mcpHost: { updateServer, clearServerToken },
      })

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_ERROR_TOKEN_UNREADABLE'),
      ).toBeInTheDocument()
      expect(screen.getByTestId('connector-token')).toBeInTheDocument()

      await click(
        screen.getByText('CHAT_UI_CONNECTOR_TOKEN_REMOVE_BUTTON_LABEL'),
      )
      await save()

      await waitFor(() =>
        expect(clearServerToken).toHaveBeenCalledWith(CALENDAR.id),
      )
    })

    it('tests the saved settings afresh', async () => {
      const { getServerTools } = await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      for (let i = 1; i <= 2; ++i) {
        await click(screen.getByText('CHAT_UI_CONNECTOR_TEST_BUTTON_LABEL'))
        expect(getServerTools).toHaveBeenCalledTimes(i)
      }
      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_TEST_SUCCESS:2'),
      ).toBeInTheDocument()
    })

    it('says the connector changed when that cancels a test', async () => {
      const observer: Observer = {}
      await renderModal({
        dialog: { view: 'editor', serverId: ISSUES.id },
        mcpHost: { getServerTools: () => new Promise(() => {}) },
        observer,
      })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')
      await click(screen.getByText('CHAT_UI_CONNECTOR_TEST_BUTTON_LABEL'))

      await act(async () => {
        observer.current!.onServersChanged([{ ...ISSUES, name: 'Renamed' }])
      })

      expect(
        await screen.findByText('CHAT_UI_CONNECTOR_ERROR_SERVER_CHANGED'),
      ).toBeInTheDocument()
      expect(
        screen.queryByText('CHAT_UI_ERROR_INTERNAL'),
      ).not.toBeInTheDocument()
    })

    it("doesn't test unsaved changes", async () => {
      await renderModal({ dialog: { view: 'editor', serverId: ISSUES.id } })
      await screen.findByText('CHAT_UI_CONNECTOR_TOKEN_SAVED')

      await fill('connector-url', 'https://mcp.example.com/other')

      expect(
        screen.getByText('CHAT_UI_CONNECTOR_TEST_BUTTON_LABEL'),
      ).toHaveProperty('isDisabled', true)
    })
  })
})
