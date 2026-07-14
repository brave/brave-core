// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/** Sentinel keys for non-vendor rail entries. */
export const PINNED_VENDOR_KEY = '__pinned__'
export const LOCAL_VENDOR_KEY = '__local__'

/** Maps Leo `display_maker` (and rail sentinels) to Leo icon names. */
export const vendorIcons: Record<string, string> = {
  [PINNED_VENDOR_KEY]: 'pin',
  [LOCAL_VENDOR_KEY]: 'laptop',
  Anthropic: 'anthropic-color',
  OpenAI: 'openai',
  Meta: 'meta-color',
  'Alibaba Cloud': 'qwen-color',
  'Z.ai': 'zai-color',
  Mistral: 'mistral-color',
  'Moonshot AI': 'kimi-color',
  Deepseek: 'deepseek-color',
  Brave: 'product-brave-leo',
}

export const fallbackVendorIcon = 'product-brave-leo'

export function getVendorIcon(vendorKey: string): string {
  return vendorIcons[vendorKey] ?? fallbackVendorIcon
}
