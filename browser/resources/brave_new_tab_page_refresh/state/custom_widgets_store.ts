/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Persistence for the AI-generated custom widgets PoC. Widgets are stored in
// localStorage only (no browser prefs/mojom) since this is a dev-only feature.

export interface CustomWidget {
  id: string
  name: string
  html: string
}

const storageKey = 'ntp-custom-widgets'

const listeners = new Set<() => void>()

function notify() {
  for (const listener of listeners) {
    listener()
  }
}

function createId() {
  if (typeof crypto !== 'undefined' && 'randomUUID' in crypto) {
    return crypto.randomUUID()
  }
  return `widget-${Date.now()}-${Math.random().toString(36).slice(2)}`
}

export function getCustomWidgets(): CustomWidget[] {
  try {
    const raw = localStorage.getItem(storageKey)
    if (!raw) {
      return []
    }
    const parsed = JSON.parse(raw)
    if (!Array.isArray(parsed)) {
      return []
    }
    return parsed.filter(
      (item): item is CustomWidget =>
        item
        && typeof item.id === 'string'
        && typeof item.name === 'string'
        && typeof item.html === 'string',
    )
  } catch {
    return []
  }
}

function save(widgets: CustomWidget[]) {
  localStorage.setItem(storageKey, JSON.stringify(widgets))
  notify()
}

export function addCustomWidget(name: string, html: string): CustomWidget {
  const widget: CustomWidget = {
    id: createId(),
    name: name.trim() || 'Custom widget',
    html,
  }
  save([...getCustomWidgets(), widget])
  return widget
}

export function removeCustomWidget(id: string) {
  save(getCustomWidgets().filter((widget) => widget.id !== id))
}

function subscribe(listener: () => void): () => void {
  listeners.add(listener)
  return () => {
    listeners.delete(listener)
  }
}

// Reflect changes made in other tabs.
window.addEventListener('storage', (event) => {
  if (event.key === storageKey) {
    notify()
  }
})

export function useCustomWidgets(): CustomWidget[] {
  const [widgets, setWidgets] = React.useState(getCustomWidgets)
  React.useEffect(() => {
    return subscribe(() => setWidgets(getCustomWidgets()))
  }, [])
  return widgets
}
