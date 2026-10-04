/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

class XStateStore {
  private snapshot: Record<string, any> | null

  constructor() {
    const reactRoot = document.querySelector('#react-root')
    const storeHost = reactRoot?.firstElementChild?.firstElementChild
    if (storeHost instanceof HTMLElement) {
      const xProps = getXProps(storeHost)
      const reduxStore = xProps?.children?.props?.store ?? {}
      this.snapshot =
        reduxStore.getState instanceof Function ? reduxStore.getState() : null
    }
  }

  access(type: string, id: string) {
    type = type === 'notifications' ? 'genericNotifications' : type
    return this.snapshot?.entities?.[type]?.entities?.[id] ?? null
  }
}

export const store = new XStateStore()

/**
 * Retrieves React-specific properties from an element by
 * finding the first property starting with "__reactProps"
 */
export function getXProps(element: HTMLElement | null): any {
  if (element instanceof HTMLElement) {
    for (const property in element) {
      if (property.startsWith('__reactProps')) {
        return element[property as keyof typeof element]
      }
    }
  }
  return null
}
