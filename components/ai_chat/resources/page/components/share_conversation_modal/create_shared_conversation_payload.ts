/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { type ConversationData } from '../../../common/conversation_serialization'
import * as Mojom from '../../../common/mojom'

/**
 * The parts of the conversation context that a shared conversation payload is
 * built from. Defined structurally, instead of as a subset of
 * ConversationContext, so that callers (including tests) can provide only this
 * data.
 */
export interface ShareableConversationContext {
  api: {
    getConversationHistory: {
      current: () => Mojom.ConversationTurn[]
    }
    getState: {
      current: () => Pick<Mojom.ConversationState, 'associatedContent'>
    }
  }
}

/**
 * The part of the AI Chat context used to resolve favicons. The UI renders
 * favicons from the browser's favicon service (chrome://favicon2), which serves
 * a different origin, so the image data has to come from the browser process.
 */
export interface FaviconContext {
  api: {
    getFaviconDataURL: {
      fetch: (pageUrl: { url: string }) => Promise<string | null>
    }
  }
}

/**
 * Resolves a page's favicon to a data URI, or `undefined` when the browser
 * doesn't have one. A missing favicon only affects how the shared conversation
 * looks, so it must never prevent sharing.
 */
async function getFaviconDataUrl(
  faviconContext: FaviconContext,
  pageUrl: string,
) {
  try {
    return (
      (await faviconContext.api.getFaviconDataURL.fetch({ url: pageUrl }))
      ?? undefined
    )
  } catch (error) {
    console.error(`Could not get the favicon for ${pageUrl}`, error)
    return undefined
  }
}

/**
 * Builds the data for a shared conversation from the conversation on display.
 *
 * Attached tabs are rendered with the page's favicon, which the local UI gets
 * from the browser's favicon service. The shared conversation viewer is a
 * regular website with no access to that service, so each favicon is resolved
 * here and inlined in the payload as a data URI.
 */
export async function createSharedConversationPayload(
  conversationContext: ShareableConversationContext,
  faviconContext: FaviconContext,
  conversationTitle: string,
): Promise<ConversationData> {
  const { api } = conversationContext

  const associatedContent = await Promise.all(
    api.getState.current().associatedContent.map(
      async (content): Promise<Mojom.AssociatedContentWithFavicon> => ({
        ...content,
        faviconUrl: await getFaviconDataUrl(faviconContext, content.url.url),
      }),
    ),
  )

  return {
    messages: api.getConversationHistory.current(),
    associatedContent,
    title: conversationTitle,
  }
}
