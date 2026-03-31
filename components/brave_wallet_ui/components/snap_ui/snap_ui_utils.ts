// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { SnapUiElement } from './snap_ui_types'

export function isSnapUiElement(value: unknown): value is SnapUiElement {
  if (value === null || typeof value !== 'object') {
    return false
  }
  const o = value as Record<string, unknown>
  return (
    typeof o.type === 'string'
    && typeof o.props === 'object'
    && o.props !== null
  )
}

export function normalizeChildList(children: unknown): unknown[] {
  if (children === undefined || children === null || children === false) {
    return []
  }
  if (Array.isArray(children)) {
    return children.flatMap((c) => normalizeChildList(c))
  }
  return [children]
}

export function reactKeyFor(element: SnapUiElement): string {
  const { type, key } = element
  if (key !== null && key !== undefined) {
    return String(key)
  }
  return `snap-${type}`
}
