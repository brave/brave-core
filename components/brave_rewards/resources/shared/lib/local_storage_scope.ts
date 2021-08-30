/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export function createLocalStorageScope<Key> (prefix: string) {
  if (prefix) {
    prefix += '-'
  }

  return {
    readJSON (key: Key): unknown {
      try {
        return JSON.parse(localStorage.getItem(prefix + key) || '')
      } catch {
        return null
      }
    },

    writeJSON (key: Key, value: unknown): void {
      localStorage.setItem(prefix + key, JSON.stringify(value))
    }
  }
}
