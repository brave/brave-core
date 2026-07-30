// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/** Sentinel keys for non-capability rail entries. */
export const PINNED_RAIL_KEY = '__pinned__'
export const ALL_RAIL_KEY = '__all__'
export const LOCAL_RAIL_KEY = '__local__'
export const OLLAMA_RAIL_KEY = '__ollama__'

/** Maps rail sentinel keys to Leo icon names. */
export const railIcons: Record<string, string> = {
  [PINNED_RAIL_KEY]: 'pin',
  [ALL_RAIL_KEY]: 'product-brave-leo',
  [LOCAL_RAIL_KEY]: 'laptop',
  [OLLAMA_RAIL_KEY]: 'ollama',
}

export function getRailIcon(railKey: string): string {
  return railIcons[railKey] ?? 'product-brave-leo'
}
