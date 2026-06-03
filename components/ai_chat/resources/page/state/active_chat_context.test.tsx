// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, act, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import {
  ActiveChatProvider,
  SelectedChatDetails,
  tabAssociatedChatId,
  useActiveChat,
} from './active_chat_context'
import { MockContext } from './mock_context'
import * as BindConversation from '../api/bind_conversation'
import {
  createMockConversationHandler,
  defaultConversationState,
} from '../api/mock_interfaces'
import createConversationAPI from '../api/conversation_api'

// Replace the real BindConversation helpers with jest mocks so we can:
// - assert which routing decision the provider made (bind by uuid vs new vs
//   tab-related bind)
// - return a working conversation API so getState.fetch() resolves and
//   `globalConversationId` actually populates inside the provider
jest.mock('../api/bind_conversation', () => ({
  __esModule: true,
  newConversation: jest.fn(),
  bindConversation: jest.fn(),
}))

const mockedBind = BindConversation as jest.Mocked<typeof BindConversation>

const makeBindings = (uuid: string): BindConversation.ConversationBindings => {
  const handler = createMockConversationHandler({
    getState: () =>
      Promise.resolve({
        conversationState: {
          ...defaultConversationState,
          conversationUuid: uuid,
        },
      }),
  })
  const conversation = createConversationAPI(handler)
  return {
    api: conversation.api,
    close: jest.fn(),
    // conversationHandler is unused by ActiveChatProvider; cast to any.
    conversationHandler: handler as any,
  }
}

interface RenderOptions {
  selectedConversationId: string | undefined
  isGlobalPanel: boolean
  updateSelectedConversationId?: jest.Mock
}

const renderProvider = async (opts: RenderOptions) => {
  const updateSelectedConversationId =
    opts.updateSelectedConversationId ?? jest.fn()

  // ActiveChatProvider gates children on its internal `conversationAPI` state
  // becoming truthy after the first bind effect. The Sink captures the
  // context value so tests can read details + invoke methods.
  const detailsRef = { current: null as SelectedChatDetails | null }
  const Sink = () => {
    detailsRef.current = useActiveChat()
    return null
  }

  const tree = (selectedConversationId: string | undefined) => (
    <MockContext aiChatOverrides={{ isGlobalPanel: opts.isGlobalPanel }}>
      <ActiveChatProvider
        selectedConversationId={selectedConversationId}
        updateSelectedConversationId={updateSelectedConversationId}
      >
        <Sink />
      </ActiveChatProvider>
    </MockContext>
  )

  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(tree(opts.selectedConversationId))
  })

  // Wait for the bind effect to fire and re-render so the Sink mounts.
  await waitFor(() => expect(detailsRef.current).not.toBeNull())
  // One more microtask cycle so getState.fetch's `.then` resolves and
  // `globalConversationId` populates in global panel mode.
  await act(async () => {
    await Promise.resolve()
  })

  const setSelectedConversationId = async (id: string | undefined) => {
    await act(async () => {
      result.rerender(tree(id))
    })
    // Flush microtasks so any post-rebind getState fetches settle.
    await act(async () => {
      await Promise.resolve()
    })
  }

  return {
    detailsRef,
    updateSelectedConversationId,
    setSelectedConversationId,
  }
}

beforeEach(() => {
  clearAllDataForTesting()
  let nextNew = 0
  let nextRelated = 0
  mockedBind.newConversation.mockReset()
  mockedBind.bindConversation.mockReset()
  mockedBind.newConversation.mockImplementation(() =>
    makeBindings(`new-${nextNew++}`),
  )
  mockedBind.bindConversation.mockImplementation((_aiChat, id) =>
    makeBindings(id ?? `related-${nextRelated++}`),
  )
  window.history.replaceState(null, '', '/')
})

describe('ActiveChatProvider routing — global panel mode', () => {
  it('binds a fresh conversation on initial load (no selected id)', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: undefined,
    })

    // No id -> newConversation -> uiHandler.newConversation in real code.
    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)
    expect(mockedBind.bindConversation).not.toHaveBeenCalled()
    expect(detailsRef.current!.isMainConversation).toBe(true)
  })

  it('binds by UUID when a specific conversation is selected', async () => {
    await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: 'history-uuid',
    })

    expect(mockedBind.bindConversation).toHaveBeenCalledWith(
      expect.anything(),
      'history-uuid',
    )
  })

  it('createNewConversation navigates to "/" when on a history conversation', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: 'history-uuid',
    })

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    expect(window.location.pathname).toBe('/')
  })

  it('createNewConversation while on the global main navigates to "/"', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: undefined,
    })

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    // In global panel mode the "main" is `selectedConversationId === undefined`,
    // so createNewConversation's tab-sentinel short-circuit doesn't apply and
    // we navigate to '/' (which will mount as a fresh global main on remount).
    expect(window.location.pathname).toBe('/')
  })

  // Regression coverage for the global-panel "new conversation" fix: while on
  // the main conversation the URL may already be '/' (or the global id), so a
  // pushState alone is a no-op and won't actually swap in a new conversation —
  // the provider must also manually rebind via newConversation.
  it('createNewConversation works after navigating to a history conversation and back to "/"', async () => {
    const { detailsRef, setSelectedConversationId } = await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: undefined,
    })

    // Initial mount in global main: one fresh bind.
    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)
    expect(detailsRef.current!.isMainConversation).toBe(true)

    // Navigate to a history conversation: should bind by uuid.
    await setSelectedConversationId('history-uuid')
    expect(mockedBind.bindConversation).toHaveBeenLastCalledWith(
      expect.anything(),
      'history-uuid',
    )
    expect(detailsRef.current!.isMainConversation).toBe(false)

    // Navigate back to the global main (URL = '/').
    await setSelectedConversationId(undefined)
    expect(detailsRef.current!.isMainConversation).toBe(true)

    mockedBind.newConversation.mockClear()
    window.history.replaceState(null, '', '/some-other-path')

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    // Must rebind manually (this is the fix) AND navigate to '/'.
    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)
    expect(window.location.pathname).toBe('/')
  })

  it('createNewConversation works after navigating to a history conversation and back to the global conversation id', async () => {
    const { detailsRef, setSelectedConversationId } = await renderProvider({
      isGlobalPanel: true,
      selectedConversationId: undefined,
    })

    // First bind in global main mode resolves to uuid 'new-0' (see mock in
    // beforeEach), which becomes the tracked globalConversationId.
    const globalConversationId = 'new-0'
    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)

    // URL set explicitly to the global conversation id is still the main
    // conversation, and should not trigger a rebind (already bound).
    mockedBind.bindConversation.mockClear()
    await setSelectedConversationId(globalConversationId)
    expect(detailsRef.current!.isMainConversation).toBe(true)
    expect(mockedBind.bindConversation).not.toHaveBeenCalled()

    // Navigate away to a history conversation.
    await setSelectedConversationId('history-uuid')
    expect(mockedBind.bindConversation).toHaveBeenLastCalledWith(
      expect.anything(),
      'history-uuid',
    )
    expect(detailsRef.current!.isMainConversation).toBe(false)

    // Navigate back to the global conversation id URL.
    await setSelectedConversationId(globalConversationId)
    expect(detailsRef.current!.isMainConversation).toBe(true)

    mockedBind.newConversation.mockClear()
    window.history.replaceState(null, '', '/some-other-path')

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    // The URL still matches the existing global conversation id, so without
    // the manual rebind a plain pushState to '/' would leave the user looking
    // at the same conversation. Verify both happen.
    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)
    expect(window.location.pathname).toBe('/')
  })
})

describe('ActiveChatProvider routing — sidebar (non-global panel) mode', () => {
  it('binds a tab-related conversation for the /tab sentinel', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: tabAssociatedChatId,
    })

    // `/tab` -> bindConversation(api, undefined) which in real code calls
    // uiHandler.bindRelatedConversation.
    expect(mockedBind.bindConversation).toHaveBeenCalledWith(
      expect.anything(),
      undefined,
    )
    expect(detailsRef.current!.isMainConversation).toBe(true)
  })

  it('binds a standalone conversation by UUID', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: 'standalone-uuid',
    })

    expect(mockedBind.bindConversation).toHaveBeenCalledWith(
      expect.anything(),
      'standalone-uuid',
    )
    expect(detailsRef.current!.isMainConversation).toBe(false)
  })

  it('createNewConversation while on /tab re-binds a fresh tab-related conversation', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: tabAssociatedChatId,
    })
    mockedBind.newConversation.mockClear()

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    expect(mockedBind.newConversation).toHaveBeenCalledTimes(1)
  })

  it('createNewConversation while on a standalone conversation navigates to "/"', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: 'standalone-uuid',
    })

    act(() => {
      detailsRef.current!.createNewConversation()
    })

    expect(window.location.pathname).toBe('/')
  })

  it('openMainConversation from /tab routes to /tab', async () => {
    const updateSpy = jest.fn()
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: tabAssociatedChatId,
      updateSelectedConversationId: updateSpy,
    })

    act(() => {
      detailsRef.current!.openMainConversation()
    })

    expect(updateSpy).toHaveBeenCalledWith(tabAssociatedChatId)
  })

  it('openMainConversation from a standalone conversation throws (no main exists)', async () => {
    const { detailsRef } = await renderProvider({
      isGlobalPanel: false,
      selectedConversationId: 'standalone-uuid',
    })

    expect(() => detailsRef.current!.openMainConversation()).toThrow(
      'No main conversation!',
    )
  })
})
