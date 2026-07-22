// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'
import { modelIcons, fallbackModelIcon } from './model_icon_map'

export function getModelIcon(model: Mojom.Model): string {
  // Check if it's an Ollama model by endpoint
  if (
    model.options?.customModelOptions?.endpoint?.url === Mojom.OLLAMA_ENDPOINT
  ) {
    return 'ollama'
  }
  return modelIcons[model.key] ?? fallbackModelIcon
}

export const AUTOMATIC_MODEL_KEY = 'chat-automatic'

export const BRAVE_SUMMARY_MODEL_KEY = 'chat-brave-summary'

export const NEAR_AI_LEARN_MORE_URL = 'https://brave.com/blog/browser-ai-tee/'

// Keep in sync with kLeoBraveSearchSupportUrl in components/ai_chat/core/browser/constants.h
export const LEO_BRAVE_SEARCH_SUPPORT_URL =
  'https://support.brave.app/hc/en-us/articles/'
  + '27586048343309-How-does-Leo-get-current-information'

/** Same destination as UntrustedUIHandler::OpenSearchURL in the browser process. */
export function getBraveSearchUrlForQuery(query: string): string {
  const url = new URL('https://search.brave.com/search')
  url.searchParams.set('q', query)
  return url.toString()
}
