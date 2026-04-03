// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

export const IGNORE_EXTERNAL_LINK_WARNING_KEY = 'IGNORE_EXTERNAL_LINK_WARNING'

const modelIcons = {
  'chat-automatic': 'product-brave-leo',
  'chat-near-glm-5': 'zai-color',
  'chat-claude-instant': 'anthropic-color',
  'chat-claude-haiku': 'anthropic-color',
  'chat-claude-sonnet': 'anthropic-color',
  'chat-qwen': 'qwen-color',
  'chat-brave-summary': 'social-brave-release-favicon-fullheight-color',
  'chat-basic': 'meta-color',
  'chat-glm-4-7-flash': 'zai-color',
  'chat-llama-4-maverick': 'meta-color',
  'chat-gpt-oss-20b': 'openai-color',
  'chat-gpt-oss-120b': 'openai-color',
  'chat-mistral-large': 'mistral-color',
  'chat-kimi-k2-5': 'kimi-color',
  'chat-qwen-3-235b': 'qwen-color',
  'chat-deepseek-v3-2': 'deepseek-color',
  'chat-qwen-3-coder-480b': 'qwen-color',
  'chat-claude-opus': 'anthropic-color',
}

const fallbackModelIcon = 'product-brave-leo'

export function getModelIcon(model: Mojom.Model): string {
  // Check if it's an Ollama model by endpoint
  if (
    model.options?.customModelOptions?.endpoint?.url === Mojom.OLLAMA_ENDPOINT
  ) {
    return 'ollama'
  }
  return modelIcons[model.key as keyof typeof modelIcons] ?? fallbackModelIcon
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
