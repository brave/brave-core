// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, render, screen, waitFor } from '@testing-library/react'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
} from '../../../common/test_data_utils'
import MockContext from '../../mock_untrusted_conversation_context'
import AssistantTask from './assistant_task'

describe('AssistantTask', () => {
  const mockAssistantEntries: Mojom.ConversationTurn[] = [
    createConversationTurnWithDefaults({
      uuid: 'test-uuid',
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [getCompletionEvent('Task in progress')],
    }),
  ]

  test('should display thumbnail when contentTaskTabId is set and task is active', async () => {
    const mockThumbnailListeners = new Map<
      number,
      (tabId: number, dataURI: string) => void
    >()
    let listenerId = 0

    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(
          (callback: (tabId: number, dataURI: string) => void) => {
            const id = ++listenerId
            mockThumbnailListeners.set(id, callback)
            return id
          },
        ),
      },
      removeListener: jest.fn((id: number) => {
        mockThumbnailListeners.delete(id)
      }),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const testTabId = 123
    const testDataURI =
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='

    render(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify thumbnail tracker was called
    expect(mockUIHandler.addTabToThumbnailTracker).toHaveBeenCalledWith(
      testTabId,
    )

    // Verify listener was added
    expect(mockUIObserver.thumbnailUpdated.addListener).toHaveBeenCalled()

    // Simulate thumbnail update
    const listener = mockThumbnailListeners.get(1)
    expect(listener).toBeDefined()
    await act(async () => listener!(testTabId, testDataURI))

    const img = screen.getByRole('img')
    expect(img).toBeInTheDocument()
    expect(img).toHaveAttribute('src', testDataURI)
  })

  test('should not display thumbnail when contentTaskTabId is not set', async () => {
    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(),
      },
      removeListener: jest.fn(),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    render(
      <MockContext
        contentTaskTabId={undefined}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify no tracker or listener setup
    expect(mockUIHandler.addTabToThumbnailTracker).not.toHaveBeenCalled()
    expect(mockUIObserver.thumbnailUpdated.addListener).not.toHaveBeenCalled()

    // Verify no image is rendered
    expect(screen.queryByRole('img')).not.toBeInTheDocument()
  })

  test('should not display thumbnail when task is not active', async () => {
    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(),
      },
      removeListener: jest.fn(),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const testTabId = 123

    render(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={false}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify no tracker or listener setup when task is not active
    expect(mockUIHandler.addTabToThumbnailTracker).not.toHaveBeenCalled()
    expect(mockUIObserver.thumbnailUpdated.addListener).not.toHaveBeenCalled()

    // Verify no image is rendered
    expect(screen.queryByRole('img')).not.toBeInTheDocument()
  })

  test('should only display thumbnail for matching tabId', async () => {
    const mockThumbnailListeners = new Map<
      number,
      (tabId: number, dataURI: string) => void
    >()
    let listenerId = 0

    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(
          (callback: (tabId: number, dataURI: string) => void) => {
            const id = ++listenerId
            mockThumbnailListeners.set(id, callback)
            return id
          },
        ),
      },
      removeListener: jest.fn((id: number) => {
        mockThumbnailListeners.delete(id)
      }),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const testTabId = 123
    const differentTabId = 456
    const testDataURI =
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='

    render(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Simulate thumbnail update for different tab
    const listener = mockThumbnailListeners.get(1)
    expect(listener).toBeDefined()
    await act(async () => listener!(differentTabId, testDataURI))

    // Verify no image appears for different tab
    expect(screen.queryByRole('img')).not.toBeInTheDocument()

    // Now simulate thumbnail update for correct tab
    await act(async () => listener!(testTabId, testDataURI))

    // Verify image appears for matching tab
    const img = screen.getByRole('img')
    expect(img).toBeInTheDocument()
    expect(img).toHaveAttribute('src', testDataURI)
  })

  test('should cleanup listeners when component unmounts', async () => {
    const mockThumbnailListeners = new Map<
      number,
      (tabId: number, dataURI: string) => void
    >()
    let listenerId = 0

    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(
          (callback: (tabId: number, dataURI: string) => void) => {
            const id = ++listenerId
            mockThumbnailListeners.set(id, callback)
            return id
          },
        ),
      },
      removeListener: jest.fn((id: number) => {
        mockThumbnailListeners.delete(id)
      }),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const testTabId = 123

    const { unmount } = render(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify initial setup
    expect(mockUIHandler.addTabToThumbnailTracker).toHaveBeenCalledWith(
      testTabId,
    )
    expect(mockUIObserver.thumbnailUpdated.addListener).toHaveBeenCalled()

    // Unmount component
    unmount()

    // Verify cleanup
    await waitFor(() => {
      expect(mockUIObserver.removeListener).toHaveBeenCalledWith(1)
      expect(mockUIHandler.removeTabFromThumbnailTracker).toHaveBeenCalledWith(
        testTabId,
      )
    })
  })

  test('should update thumbnail tracker when contentTaskTabId changes', async () => {
    const mockThumbnailListeners = new Map<
      number,
      (tabId: number, dataURI: string) => void
    >()
    let listenerId = 0

    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(
          (callback: (tabId: number, dataURI: string) => void) => {
            const id = ++listenerId
            mockThumbnailListeners.set(id, callback)
            return id
          },
        ),
      },
      removeListener: jest.fn((id: number) => {
        mockThumbnailListeners.delete(id)
      }),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const initialTabId = 123
    const newTabId = 456

    const { rerender } = render(
      <MockContext
        contentTaskTabId={initialTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify initial setup
    expect(mockUIHandler.addTabToThumbnailTracker).toHaveBeenCalledWith(
      initialTabId,
    )

    // Change contentTaskTabId
    rerender(
      <MockContext
        contentTaskTabId={newTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify cleanup of old tab and setup of new tab
    await waitFor(() => {
      expect(mockUIHandler.removeTabFromThumbnailTracker).toHaveBeenCalledWith(
        initialTabId,
      )
      expect(mockUIHandler.addTabToThumbnailTracker).toHaveBeenCalledWith(
        newTabId,
      )
    })
  })

  test('should cleanup thumbnail tracker when isActiveTask changes to false', async () => {
    const mockThumbnailListeners = new Map<
      number,
      (tabId: number, dataURI: string) => void
    >()
    let listenerId = 0

    const mockUIObserver = {
      thumbnailUpdated: {
        addListener: jest.fn(
          (callback: (tabId: number, dataURI: string) => void) => {
            const id = ++listenerId
            mockThumbnailListeners.set(id, callback)
            return id
          },
        ),
      },
      removeListener: jest.fn((id: number) => {
        mockThumbnailListeners.delete(id)
      }),
    }

    const mockUIHandler = {
      addTabToThumbnailTracker: jest.fn(),
      removeTabFromThumbnailTracker: jest.fn(),
    }

    const testTabId = 123

    const { rerender } = render(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify initial setup
    expect(mockUIHandler.addTabToThumbnailTracker).toHaveBeenCalledWith(
      testTabId,
    )

    // Change isActiveTask to false
    rerender(
      <MockContext
        contentTaskTabId={testTabId}
        uiHandler={mockUIHandler as unknown as Mojom.UntrustedUIHandlerRemote}
        uiObserver={
          mockUIObserver as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={false}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify cleanup
    await waitFor(() => {
      expect(mockUIObserver.removeListener).toHaveBeenCalledWith(1)
      expect(mockUIHandler.removeTabFromThumbnailTracker).toHaveBeenCalledWith(
        testTabId,
      )
    })
  })
})
