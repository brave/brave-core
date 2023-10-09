// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type CacheListener<T> = (
  newValue: T,
  oldValue: T
) => void

/**
 * Allows consumers to subscribe to changes to an object.
 */
export class CachingWrapper<T> {
  protected cache: T
  protected listeners: Array<CacheListener<T>> = []

  constructor(defaultValue: T) {
    this.cache = defaultValue;
  }

  addListener (listener: CacheListener<T>, init = true) {
    this.listeners.push(listener)
    if (init) listener(this.cache, {} as any)
  }

  removeListener (listener: CacheListener<T>) {
    this.listeners = this.listeners.filter((l) => l !== listener)
  }

  change (newValue: T) {
    const oldValue = this.cache
    this.cache = newValue

    for (const listener of this.listeners) {
      listener(newValue, oldValue)
    }
  }
}

export type Cache<Entity> = { [key: string]: Entity }
export type ChangeEvent<Entity> = {
  addedOrUpdated: { [key: string]: Entity }
  removed: string[]
}

/**
 * A helper class for caching a dictionary of entities which may be
 * added/updated/remove.
 *
 * Manages partial updates and notifies listeners of the changes.
 */
export class EntityCachingWrapper<Entity> extends CachingWrapper<Cache<Entity>> {
  constructor() {
    super({})
  }

  changed (event: ChangeEvent<Entity>) {
    const copy: Cache<Entity> = { ...this.cache }
    for (const id in event.addedOrUpdated) copy[id] = event.addedOrUpdated[id]
    for (const id of event.removed) delete copy[id]

    this.change(copy)
  }
}
