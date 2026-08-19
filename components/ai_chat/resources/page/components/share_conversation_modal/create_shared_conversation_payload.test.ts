// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { parseConversationData } from '../../../common/conversation_serialization'
import * as Mojom from '../../../common/mojom'
import sampleConversationJson from '../../stories/conversations/multi_tool_multi_turn'
import {
  createSharedConversationPayload,
  type FaviconContext,
  type ShareableConversationContext,
} from './create_shared_conversation_payload'

// The story fixture is stored in its serialized form, so revive it to get the
// mojom types a conversation actually holds.
const SampleConversation = parseConversationData(
  JSON.stringify(sampleConversationJson),
).messages

function createAssociatedContent(
  contentId: number,
  url: string,
): Mojom.AssociatedContent {
  return {
    uuid: `associated-content-${contentId}`,
    contentType: Mojom.ContentType.PageContent,
    contentId,
    title: `Page ${contentId}`,
    url: { url },
    contentUsedPercentage: 100,
    conversationTurnUuid: SampleConversation[0].uuid,
    toolsAttached: false,
  }
}

function createConversationContext(
  associatedContent: Mojom.AssociatedContent[],
): ShareableConversationContext {
  return {
    api: {
      getConversationHistory: { current: () => SampleConversation },
      getState: { current: () => ({ associatedContent }) },
    },
  }
}

const faviconDataUrl = 'data:image/png;base64,AQID'

describe('createSharedConversationPayload', () => {
  let getFaviconDataURL: jest.Mock
  let faviconContext: FaviconContext

  beforeEach(() => {
    getFaviconDataURL = jest.fn()
    faviconContext = {
      api: { getFaviconDataURL: { fetch: getFaviconDataURL } },
    }
  })

  it('includes the conversation messages, title and associated content', async () => {
    getFaviconDataURL.mockResolvedValue(faviconDataUrl)
    const content = createAssociatedContent(1, 'https://example.com/1')

    const payload = await createSharedConversationPayload(
      createConversationContext([content]),
      faviconContext,
      'A shared conversation',
    )

    expect(payload.messages).toEqual(SampleConversation)
    expect(payload.title).toBe('A shared conversation')
    expect(payload.associatedContent).toEqual([
      { ...content, faviconUrl: faviconDataUrl },
    ])
  })

  it('inlines the favicon of each associated content item', async () => {
    getFaviconDataURL.mockResolvedValue(faviconDataUrl)
    const contents = [
      createAssociatedContent(1, 'https://example.com/1'),
      createAssociatedContent(2, 'https://example.com/2'),
    ]

    const payload = await createSharedConversationPayload(
      createConversationContext(contents),
      faviconContext,
      'A shared conversation',
    )

    // The browser is asked for the favicon of every attached page.
    expect(getFaviconDataURL.mock.calls.map(([pageUrl]) => pageUrl)).toEqual([
      { url: 'https://example.com/1' },
      { url: 'https://example.com/2' },
    ])
    expect(payload.associatedContent.map((c) => c.faviconUrl)).toEqual([
      faviconDataUrl,
      faviconDataUrl,
    ])
  })

  it('shares the conversation without the favicons it cannot retrieve', async () => {
    // No favicon stored for the page, and a failed call, should both leave the
    // content without a favicon rather than fail the share.
    getFaviconDataURL.mockResolvedValueOnce(null)
    getFaviconDataURL.mockRejectedValueOnce(new Error('Disconnected'))
    getFaviconDataURL.mockResolvedValueOnce(faviconDataUrl)
    const contents = [
      createAssociatedContent(1, 'https://example.com/1'),
      createAssociatedContent(2, 'https://example.com/2'),
      createAssociatedContent(3, 'https://example.com/3'),
    ]
    jest.spyOn(console, 'error').mockImplementation(() => {})

    const payload = await createSharedConversationPayload(
      createConversationContext(contents),
      faviconContext,
      'A shared conversation',
    )

    expect(payload.associatedContent).toEqual([
      { ...contents[0], faviconUrl: undefined },
      { ...contents[1], faviconUrl: undefined },
      { ...contents[2], faviconUrl: faviconDataUrl },
    ])
  })

  it('does not request favicons when there is no associated content', async () => {
    const payload = await createSharedConversationPayload(
      createConversationContext([]),
      faviconContext,
      'A shared conversation',
    )

    expect(payload.associatedContent).toEqual([])
    expect(getFaviconDataURL).not.toHaveBeenCalled()
  })
})
