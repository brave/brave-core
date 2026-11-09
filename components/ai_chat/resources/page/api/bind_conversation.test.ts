// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AIChatAPI } from './ai_chat_api'
import { bindConversation, newConversation } from './bind_conversation'

const mockSetClientVisible = jest.fn()

// Real remotes and receivers need the Mojo JS runtime, which jest doesn't have.
jest.mock('../../common/mojom', () => ({
  ...jest.requireActual('../../common/mojom'),
  ConversationHandlerRemote: class {
    $ = { bindNewPipeAndPassReceiver: () => ({}), close: () => {} }
    setClientVisible(visible: boolean) {
      mockSetClientVisible(visible)
    }
  },
  ConversationUIReceiver: class {
    $ = { bindNewPipeAndPassRemote: () => ({}), close: () => {} }
  },
}))

jest.mock('./conversation_api', () => ({
  __esModule: true,
  default: () => ({
    api: { getState: { update: () => {} } },
    conversationUIObserver: {},
    close: () => {},
  }),
}))

function makeAIChat() {
  return {
    service: { bindConversation: jest.fn() },
    uiHandler: {
      bindRelatedConversation: jest.fn(),
      newConversation: jest.fn(),
    },
  } as unknown as AIChatAPI['api']
}

let visibilityState: DocumentVisibilityState = 'visible'

function setVisibility(state: DocumentVisibilityState) {
  visibilityState = state
  document.dispatchEvent(new Event('visibilitychange'))
}

beforeAll(() => {
  Object.defineProperty(document, 'visibilityState', {
    configurable: true,
    get: () => visibilityState,
  })
})

afterAll(() => {
  delete (document as { visibilityState?: unknown }).visibilityState
})

describe.each([
  [
    'a stored conversation',
    (aiChat: AIChatAPI['api']) => bindConversation(aiChat, 'uuid-1'),
  ],
  [
    "the tab's conversation",
    (aiChat: AIChatAPI['api']) => bindConversation(aiChat, undefined),
  ],
  ['a new conversation', (aiChat: AIChatAPI['api']) => newConversation(aiChat)],
])('binding %s', (_name, bind) => {
  // Closed after each test, even a failed one, so its visibility listener
  // can't report into the next.
  let binding: { close: () => void } | undefined

  beforeEach(() => {
    mockSetClientVisible.mockClear()
    visibilityState = 'visible'
  })

  afterEach(() => {
    binding?.close()
    binding = undefined
  })

  it('reports the page is visible when it binds', () => {
    binding = bind(makeAIChat())
    expect(mockSetClientVisible.mock.calls).toEqual([[true]])
  })

  it('reports the page is hidden when it binds in the background', () => {
    visibilityState = 'hidden'
    binding = bind(makeAIChat())
    expect(mockSetClientVisible.mock.calls).toEqual([[false]])
  })

  it('reports every visibility change until it is closed', () => {
    binding = bind(makeAIChat())
    setVisibility('hidden')
    setVisibility('visible')
    binding.close()
    setVisibility('hidden')
    expect(mockSetClientVisible.mock.calls).toEqual([[true], [false], [true]])
  })
})
