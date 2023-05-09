// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type ChangeEvent<Entity> = {
  addedOrUpdated: { [key: string]: Entity }
  removed: string[]
}

export type Cache<Entity> = { [key: string]: Entity }

export type CacheListener<Entity> = (
  newValue: Cache<Entity>,
  oldValue: Cache<Entity>
) => void

export class CachingWrapper<Entity> {
  protected cache: Cache<Entity> = {}
  protected listeners: Array<CacheListener<Entity>> = []

  changed (event: ChangeEvent<Entity>) {
    const copy = { ...this.cache }
    for (const id in event.addedOrUpdated) copy[id] = event.addedOrUpdated[id]
    for (const id of event.removed) delete copy[id]

    this.change(copy)
  }

  addListener (listener: CacheListener<Entity>, init = true) {
    this.listeners.push(listener)
    if (init) listener(this.cache, {})
  }

  removeListener (listener: CacheListener<Entity>) {
    this.listeners = this.listeners.filter((l) => l !== listener)
  }

  change (newValue: { [key: string]: Entity }) {
    const oldValue = this.cache
    this.cache = newValue

    for (const listener of this.listeners) {
      listener(newValue, oldValue)
    }
  }
}
