// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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

export function getModelIcon(modelKey: string): string {
  return modelIcons[modelKey as keyof typeof modelIcons] ?? fallbackModelIcon
}

export const AUTOMATIC_MODEL_KEY = 'chat-automatic'

export const NEAR_AI_LEARN_MORE_URL = 'https://docs.near.ai/private-ml-sdk/'
