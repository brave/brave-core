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
  'chat-llama-4-scout': 'meta-color',
  'chat-llama-4-maverick': 'meta-color',
  'chat-gpt-oss-20b': 'openai-color',
  'chat-gpt-oss-120b': 'openai-color',
  'chat-mistral-large': 'mistral-color',
  'chat-pixtral-large': 'mistral-color',
  'chat-qwen-3-235b': 'qwen-color',
  'chat-deepseek-v3-1': 'deepseek-color',
  'chat-qwen-3-coder-480b': 'qwen-color',
  'chat-claude-opus': 'anthropic-color',
}

const fallbackModelIcon = 'product-brave-leo'

export function getModelIcon(modelKey: string): string {
  return modelIcons[modelKey as keyof typeof modelIcons] ?? fallbackModelIcon
}
