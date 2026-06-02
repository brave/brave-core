// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Maps `model.key` to a Leo icon name. Shared with chrome://settings/leo via
// a preprocess_if_expr step in //brave/browser/resources/settings/BUILD.gn.
export const modelIcons: Record<string, string> = {
  'chat-automatic': 'product-brave-leo',
  'chat-near-glm-5': 'zai-color',
  'chat-near-glm-5-1': 'zai-color',
  'chat-claude-instant': 'anthropic-color',
  'chat-claude-haiku': 'anthropic-color',
  'chat-claude-sonnet': 'anthropic-color',
  'chat-qwen': 'qwen-color',
  'chat-brave-summary': 'social-brave-release-favicon-fullheight-color',
  'chat-basic': 'meta-color',
  'chat-glm-4-7-flash': 'zai-color',
  'chat-gpt-oss-20b': 'openai',
  'chat-gpt-5-4-bedrock': 'openai',
  'chat-nemotron-nano-3-30b': 'nvidia-color',
  'chat-mistral-large': 'mistral-color',
  'chat-kimi-k2-5': 'kimi-color',
  'chat-qwen-3-235b': 'qwen-color',
  'chat-deepseek-v3-2': 'deepseek-color',
  'chat-claude-opus': 'anthropic-color',
}

export const fallbackModelIcon = 'product-brave-leo'
