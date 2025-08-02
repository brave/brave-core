// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// https://docs.aws.amazon.com/bedrock/latest/APIReference/API_runtime_Message.html#API_runtime_Message_Contents
export const MAX_IMAGES = 20
export const MAX_DOCUMENTS = 5
export const MAX_DOCUMENT_SIZE_BYTES = 4.5 * 1024 * 1024 // 4.5MB in bytes

export const IGNORE_EXTERNAL_LINK_WARNING_KEY = 'IGNORE_EXTERNAL_LINK_WARNING'

const modelIcons = {
  'chat-automatic': 'product-brave-leo',
  'chat-deepseek-r1': 'deepseek-color',
  'chat-claude-instant': 'anthropic-color',
  'chat-claude-haiku': 'anthropic-color',
  'chat-claude-sonnet': 'anthropic-color',
  'chat-qwen': 'qwen-color',
  'chat-basic': 'meta-color',
  'chat-gemma': 'google-color',
}

const fallbackModelIcon = 'product-brave-leo'

export function getModelIcon(modelKey: string): string {
  return modelIcons[modelKey as keyof typeof modelIcons] ?? fallbackModelIcon
}
