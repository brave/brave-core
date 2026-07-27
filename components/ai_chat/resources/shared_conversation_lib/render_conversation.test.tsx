// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as fs from 'node:fs'
import * as path from 'node:path'
import { act, screen } from '@testing-library/react'
import '@testing-library/jest-dom'
import { renderConversation } from './render_conversation'

// render_conversation sets the Leo icon base path from `import.meta.url`, which
// the CommonJS-based unit test runner cannot parse. That side effect is
// irrelevant to these tests, so mock the module out (this is hoisted above the
// render_conversation import below).
jest.mock('./set_icon_base_path', () => ({}))

// A real conversation exported from the AI Chat "Share conversation" action,
// in the format expected by renderConversation (see conversation_serialization).
const conversationData = fs.readFileSync(
  path.join(__dirname, 'render_conversation.test-data.txt'),
  'utf8',
)

describe('renderConversation', () => {
  let container: HTMLElement

  beforeAll(() => {
    // jsdom does not implement Element.prototype.scrollTo, which the real
    // useScrollToBottom hook calls when the conversation mounts. Use a plain
    // (non-mock) stub so it survives Jest's per-test mock resets.
    Element.prototype.scrollTo = () => {}
  })

  beforeEach(() => {
    container = document.createElement('div')
    document.body.appendChild(container)
  })

  afterEach(() => {
    container.remove()
  })

  const render = () => {
    act(() => {
      renderConversation(conversationData, container)
    })
  }

  it('shows the conversation when given data', async () => {
    render()

    // The human turn from the sample data should be rendered.
    expect(
      await screen.findByText('weather in santa barbara'),
    ).toBeInTheDocument()
  })

  it('does not show feedback buttons (read-only)', async () => {
    render()

    // Ensure the conversation - including its assistant turn - has rendered,
    // so the (suppressed) feedback buttons would otherwise be present.
    await screen.findByText('weather in santa barbara')
    expect(await screen.findByTestId('assistant-turn')).toBeInTheDocument()

    // renderConversation renders read-only, so the assistant feedback buttons
    // (like / dislike, from ContextActionsAssistant) must not be present. Their
    // `title` is the string key, since getLocale is mocked to echo the key.
    expect(
      screen.queryByTitle(S.CHAT_UI_LIKE_ANSWER_BUTTON_LABEL),
    ).not.toBeInTheDocument()
    expect(
      screen.queryByTitle(S.CHAT_UI_DISMISS_BUTTON_LABEL),
    ).not.toBeInTheDocument()
  })
})
