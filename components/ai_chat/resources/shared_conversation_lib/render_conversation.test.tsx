// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as fs from 'node:fs'
import * as path from 'node:path'
import { act, screen } from '@testing-library/react'
import '@testing-library/jest-dom'
import * as Mojom from '../common/mojom'
import {
  parseConversationData,
  stringifyConversationData,
} from '../common/conversation_serialization'
import { renderConversation } from './render_conversation'

// render_conversation sets the Leo icon base path from `import.meta.url`, which
// the CommonJS-based unit test runner cannot parse. That side effect is
// irrelevant to these tests, so mock the module out (this is hoisted above the
// render_conversation import below).
jest.mock('./set_icon_base_path', () => ({}))
// No shadow DOM support in tests
jest.mock('./setup_rendering_element', () => ({
  __esModule: true,
  default: (element: HTMLElement) => element,
}))

// A real conversation exported from the AI Chat "Share conversation" action,
// in the format expected by renderConversation (see conversation_serialization).
// It predates associated content being part of the payload, so it also covers
// rendering conversations shared by an older client.
const conversationData = fs.readFileSync(
  path.join(__dirname, 'render_conversation.test-data.txt'),
  'utf8',
)

const sharedConversation = parseConversationData(conversationData)

// Attachments are only rendered on the turn they were submitted with, so the
// associated content must point at the sample data's human turn.
const humanTurn = sharedConversation.messages.find(
  (message) => message.characterType === Mojom.CharacterType.HUMAN,
)!

// The favicon comes with the payload as a data URI, because the viewer has no
// access to the browser's favicon service.
const attachedTabFaviconUrl =
  'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7'

const attachedTab: Mojom.AssociatedContentWithFavicon = {
  uuid: 'e6b3f6c8-0f2a-4a1e-9d16-3f4f1f7b5c21',
  contentType: Mojom.ContentType.PageContent,
  contentId: 1,
  title: 'Santa Barbara Weather Forecast',
  url: { url: 'https://weather.example.com/santa-barbara' },
  contentUsedPercentage: 100,
  conversationTurnUuid: humanTurn.uuid,
  toolsAttached: false,
  faviconUrl: attachedTabFaviconUrl,
}

// The same conversation as above, as a current client would share it: with the
// content that was attached to the human turn.
const conversationDataWithAttachedTab = stringifyConversationData({
  ...sharedConversation,
  associatedContent: [attachedTab],
})

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

  const render = (data = conversationData) => {
    act(() => {
      renderConversation(data, container)
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

  it('shows the attached tabs of the associated content', async () => {
    render(conversationDataWithAttachedTab)

    // AttachmentPageItem shows the page title, and the URL without its scheme.
    expect(
      await screen.findByText(attachedTab.title, { selector: '.title' }),
    ).toBeInTheDocument()
    expect(
      screen.getByText('weather.example.com/santa-barbara'),
    ).toBeInTheDocument()
  })

  it('shows the attached tab favicon provided in the conversation', async () => {
    render(conversationDataWithAttachedTab)

    await screen.findByText(attachedTab.title, { selector: '.title' })

    expect(
      container.querySelector(`.favicon img[src="${attachedTabFaviconUrl}"]`),
    ).toBeInTheDocument()
    // The browser's favicon service is not available to the viewer, so it must
    // not be relied on for any image.
    expect(
      container.querySelector('img[src*="favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('shows conversations shared without associated content', async () => {
    render()

    await screen.findByText('weather in santa barbara')

    // No attachments to display, and rendering the conversation should not be
    // affected by their absence.
    expect(container.querySelector('.itemWrapper')).not.toBeInTheDocument()
  })
})
