// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { renderHook, act, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../common/mojom'
import { defaultConversationState } from '../api/mock_interfaces'
import { useAIChat } from './ai_chat_context'
import { useConversation } from './conversation_context'
import { MockContext } from './mock_context'

const CONVERSATION_UUID = defaultConversationState.conversationUuid

const tab = (contentId: number): Mojom.TabData => ({
  id: contentId,
  contentId,
  title: `Tab ${contentId}`,
  url: { url: `https://example.com/${contentId}` },
})

const stagedContent = (contentId: number): Mojom.AssociatedContent => ({
  uuid: `assoc-${contentId}`,
  contentType: Mojom.ContentType.PageContent,
  title: `Tab ${contentId}`,
  contentId,
  url: { url: `https://example.com/${contentId}` },
  contentUsedPercentage: 0,
  conversationTurnUuid: undefined,
  toolsAttached: false,
})

const committedContent = (
  contentId: number,
  turnUuid: string,
): Mojom.AssociatedContent => ({
  ...stagedContent(contentId),
  conversationTurnUuid: turnUuid,
})

interface RenderOverrides {
  associateTab?: jest.Mock
  disassociateContent?: jest.Mock
}

const renderConversation = (overrides: RenderOverrides = {}) =>
  renderHook(
    () => ({
      conversation: useConversation(),
      aiChat: useAIChat(),
    }),
    {
      wrapper: ({ children }: { children: React.ReactNode }) => (
        <MockContext
          uiHandler={{
            associateTab: overrides.associateTab ?? jest.fn(),
            disassociateContent: overrides.disassociateContent ?? jest.fn(),
          }}
          initialState={{ isStandalone: false }}
          conversationProps={{ isMainConversation: true }}
          aiChatOverrides={{ isGlobalPanel: true }}
        >
          {children}
        </MockContext>
      ),
    },
  )

// Drives the state the effect needs:
// - conversationUuid populated (the effect bails out without it; the mock's
//   getState() resolves asynchronously, so we wait for it)
// - tabsData populated (so the contentId can resolve to a TabData)
// - defaultTabContentId emitted (the trigger)
const setUpAttached = async (
  rendered: ReturnType<typeof renderConversation>,
  contentId: number,
) => {
  await act(async () => {
    await rendered.result.current.conversation.api.getState.fetch()
  })
  await act(async () => {
    rendered.result.current.aiChat.api.tabs.update([tab(contentId)])
  })
  await act(async () => {
    rendered.result.current.aiChat.api.emitEvent('onNewDefaultConversation', [
      contentId,
    ])
  })
  await act(async () => rendered.rerender())
}

describe('useProvideConversationContext default tab attachment', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('attaches the default tab once everything it needs is ready', async () => {
    const associateTab = jest.fn()
    const rendered = renderConversation({ associateTab })

    await setUpAttached(rendered, 10)

    expect(associateTab).toHaveBeenCalledTimes(1)
    expect(associateTab).toHaveBeenCalledWith(tab(10), CONVERSATION_UUID)
  })

  // Regression test for the global side panel "can't remove the current tab"
  // bug: after the user disassociates the default tab, associatedContent
  // changes, but that must not cause the attach effect to fire again.
  it('does not re-attach the default tab after the user removes it', async () => {
    const associateTab = jest.fn()
    const rendered = renderConversation({ associateTab })
    const { result } = rendered

    await setUpAttached(rendered, 10)

    expect(associateTab).toHaveBeenCalledTimes(1)
    associateTab.mockClear()

    // Backend confirms the staged attachment.
    await act(async () => {
      result.current.conversation.api.getState.update({
        ...defaultConversationState,
        associatedContent: [stagedContent(10)],
      })
    })

    // User removes the tab — backend reflects empty associated content.
    await act(async () => {
      result.current.conversation.api.getState.update({
        ...defaultConversationState,
        associatedContent: [],
      })
    })

    expect(associateTab).not.toHaveBeenCalled()
  })

  // Also part of the same regression: tabsData updating (e.g. tab title
  // changes) must not undo the user's removal.
  it('does not re-attach when tabsData updates after the user removes the tab', async () => {
    const associateTab = jest.fn()
    const rendered = renderConversation({ associateTab })
    const { result } = rendered

    await setUpAttached(rendered, 10)

    expect(associateTab).toHaveBeenCalledTimes(1)
    associateTab.mockClear()

    await act(async () => {
      result.current.conversation.api.getState.update({
        ...defaultConversationState,
        associatedContent: [],
      })
    })

    await act(async () => {
      result.current.aiChat.api.tabs.update([
        { ...tab(10), title: 'Tab 10 (renamed)' },
      ])
    })

    expect(associateTab).not.toHaveBeenCalled()
  })

  it('attaches once tabsData catches up after the default tab event', async () => {
    const associateTab = jest.fn()
    const rendered = renderConversation({ associateTab })
    const { result } = rendered

    await act(async () => {
      await result.current.conversation.api.getState.fetch()
    })

    // OnNewDefaultConversation arrives before tabsData reflects the new tab.
    await act(async () => {
      result.current.aiChat.api.emitEvent('onNewDefaultConversation', [20])
    })
    await act(async () => rendered.rerender())

    expect(associateTab).not.toHaveBeenCalled()

    await act(async () => {
      result.current.aiChat.api.tabs.update([tab(20)])
    })
    await act(async () => rendered.rerender())

    expect(associateTab).toHaveBeenCalledTimes(1)
    expect(associateTab).toHaveBeenCalledWith(tab(20), CONVERSATION_UUID)
  })

  // When the user navigates from an attachable page (e.g. https://) to one
  // that can't be associated (e.g. chrome://), defaultTabContentId becomes
  // undefined. All staged content should be cleared; committed content (tied
  // to a conversation turn) must be preserved.
  it('clears staged content when defaultTabContentId becomes undefined', async () => {
    const disassociateContent = jest.fn()
    const rendered = renderConversation({ disassociateContent })
    const { result } = rendered

    await setUpAttached(rendered, 10)

    const stagedDefault = stagedContent(10)
    const stagedOther = stagedContent(99)
    const committed = committedContent(7, 'turn-1')
    await act(async () => {
      result.current.conversation.api.getState.update({
        ...defaultConversationState,
        associatedContent: [stagedDefault, stagedOther, committed],
      })
    })

    disassociateContent.mockClear()

    await act(async () => {
      result.current.aiChat.api.emitEvent('onNewDefaultConversation', [])
    })
    await act(async () => rendered.rerender())

    expect(disassociateContent).toHaveBeenCalledWith(
      stagedDefault,
      CONVERSATION_UUID,
    )
    expect(disassociateContent).toHaveBeenCalledWith(
      stagedOther,
      CONVERSATION_UUID,
    )
    expect(disassociateContent).not.toHaveBeenCalledWith(
      committed,
      CONVERSATION_UUID,
    )
  })

  it('attachFiles flips isUploadingFiles to true while running, and back to false when done', async () => {
    // Hold the processXFile call open via a controllable resolver so we can
    // observe both the rising and falling edge of isUploadingFiles.
    let resolveProcess:
      | ((value: { processedFile: Mojom.UploadedFile | null }) => void)
      | undefined
    const rendered = renderHook(() => useConversation(), {
      wrapper: ({ children }: { children: React.ReactNode }) => (
        <MockContext
          uiHandler={{
            processTextFile: () =>
              new Promise((resolve) => {
                resolveProcess = resolve
              }),
          }}
        >
          {children}
        </MockContext>
      ),
    })

    expect(rendered.result.current.isUploadingFiles).toBe(false)

    // Fire-and-forget so the mutation stays pending while we assert.
    let attachPromise: Promise<unknown> = Promise.resolve()
    act(() => {
      attachPromise = rendered.result.current.attachFiles([
        new File(['hi'], 'foo.txt', { type: 'text/plain' }),
      ])
    })

    // attachFiles awaits file.arrayBuffer() before calling mutateAsync, so
    // the rising edge is a couple of microtasks away — wait for it.
    await waitFor(() =>
      expect(rendered.result.current.isUploadingFiles).toBe(true),
    )

    // Resolve the pending mutation; isUploadingFiles must flip back.
    await act(async () => {
      resolveProcess!({ processedFile: null })
      await attachPromise
    })
    await waitFor(() =>
      expect(rendered.result.current.isUploadingFiles).toBe(false),
    )
  })

  it.each([
    ['text', 'processTextFile' as const, 'text/plain', 'foo.txt'],
    ['image', 'processImageFile' as const, 'image/png', 'foo.png'],
    ['pdf', 'processPdfFile' as const, 'application/pdf', 'foo.pdf'],
  ])(
    'isUploadingFiles stays true while a %s file mutation is pending',
    async (_label, methodName, mime, filename) => {
      let resolveProcess:
        | ((value: { processedFile: Mojom.UploadedFile | null }) => void)
        | undefined
      const rendered = renderHook(() => useConversation(), {
        wrapper: ({ children }: { children: React.ReactNode }) => (
          <MockContext
            uiHandler={{
              [methodName]: () =>
                new Promise((resolve) => {
                  resolveProcess = resolve
                }),
            }}
          >
            {children}
          </MockContext>
        ),
      })

      let attachPromise: Promise<unknown> = Promise.resolve()
      act(() => {
        attachPromise = rendered.result.current.attachFiles([
          new File(['hi'], filename, { type: mime }),
        ])
      })

      // While the specific processXFile mutation is pending, isUploadingFiles
      // should be true.
      await waitFor(() =>
        expect(rendered.result.current.isUploadingFiles).toBe(true),
      )
      // And it stays true as long as we don't resolve.
      expect(rendered.result.current.isUploadingFiles).toBe(true)

      // Resolve the mutation; isUploadingFiles flips back to false.
      await act(async () => {
        resolveProcess!({ processedFile: null })
        await attachPromise
      })
      await waitFor(() =>
        expect(rendered.result.current.isUploadingFiles).toBe(false),
      )
    },
  )

  it('clears staged content and attaches the new tab when the default changes', async () => {
    const associateTab = jest.fn()
    const disassociateContent = jest.fn()
    const rendered = renderConversation({
      associateTab,
      disassociateContent,
    })
    const { result } = rendered

    await setUpAttached(rendered, 10)

    await act(async () => {
      result.current.conversation.api.getState.update({
        ...defaultConversationState,
        associatedContent: [stagedContent(10)],
      })
    })

    associateTab.mockClear()
    disassociateContent.mockClear()

    await act(async () => {
      result.current.aiChat.api.tabs.update([tab(10), tab(20)])
    })
    await act(async () => {
      result.current.aiChat.api.emitEvent('onNewDefaultConversation', [20])
    })
    await act(async () => rendered.rerender())

    expect(disassociateContent).toHaveBeenCalledWith(
      stagedContent(10),
      CONVERSATION_UUID,
    )
    expect(associateTab).toHaveBeenCalledTimes(1)
    expect(associateTab).toHaveBeenCalledWith(tab(20), CONVERSATION_UUID)
  })
})
