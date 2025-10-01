// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

export const IGNORE_EXTERNAL_LINK_WARNING_KEY = 'IGNORE_EXTERNAL_LINK_WARNING'

const modelIcons = {
  'chat-automatic': 'product-brave-leo',
  'chat-deepseek-r1': 'deepseek-color',
  'chat-claude-instant': 'anthropic-color',
  'chat-claude-haiku': 'anthropic-color',
  'chat-claude-sonnet': 'anthropic-color',
  'chat-qwen': 'qwen-color',
  'chat-basic': 'meta-color',
  'chat-gemma': 'gemma-color',
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
