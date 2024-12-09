// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type CacheListener<T> = (
  newValue: T,
  oldValue: T
) => void

export function immutableMerge<T extends { [key: string | number]: any; }>(initial: T, patch: Partial<T>): T {
  if (patch === undefined) return initial

  const result: T = { ...initial }

  for (const key in patch) {
    const patchedValue = patch[key]
    const patchedType = typeof patchedValue

    if (Array.isArray(patchedValue)) {
      const array = [...initial as any]
      for (const [index, value] of patchedValue.entries()) {
        if (value === null) {
          delete array[index]
        } else {
          array[index] = value
        }
      }
      result[key as any] = array
    } else if (patchedType === 'object') {
      for (const [key, value] of Object.entries(patchedValue as any)) {
        result[key as any] = value
      }
    } else {
      result[key as any] = patch[key]
    }
  }

  return result
}

/**
 * Allows consumers to subscribe to changes to an object.
 *
 * This is useful for listening to changes to a remote mojom object and
 * keeping track of the intermediate value.
 */
export class CachingWrapper<T> {
  protected cache: T
  protected listeners: Array<CacheListener<T>> = []

  constructor(defaultValue: T) {
    this.cache = defaultValue;
  }

  addListener(listener: CacheListener<T>, init = true) {
    this.listeners.push(listener)
    if (init) listener(this.cache, {} as any)
  }

  removeListener(listener: CacheListener<T>) {
    this.listeners = this.listeners.filter((l) => l !== listener)
  }

  /**
   * Notifies all listeners that the latest value has been updated.
   * @param newValue The new value
   */
  notifyChanged(newValue: T) {
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
 *
 * Example:
 *
 * in example.mojom:
 *
 * struct ExampleEvent {
 *   map<string, Entity> addedOrUpdated;
 *   array<string> removed;
 * };
 *
 * interface ExampleListener {
 *   Changed(ExampleEvent event);
 * };
 *
 * interface Controller {
 *   AddListener(pending_remote<ExampleListener> listener);
 * }
 *
 * in exampleCache.ts
 *
 * class ExampleCache
 *  extends EntityCachingWrapper<Entity>
 *  implements ExampleListenerInterface {
 *   private receiver = new ExampleListenerReceiver(this)
 *
 *   constructor() {
 *     super()
 *
 *     // where get controller gets you a bound instance of Controller
 *     getController().addExampleListener(this.receiver.$.bindNewPipeAndPassRemove())
 *   }
 * }
 *
 * // and use the cache
 * const cache = new ExampleCache()
 * cache.addListener(console.log)
 */
export class EntityCachingWrapper<Entity> extends CachingWrapper<Cache<Entity>> {
  constructor() {
    super({})
  }

  // Note: This function is called via mojom - it should be defined on the
  // listener interface.
  changed(event: ChangeEvent<Entity>) {
    const copy: Cache<Entity> = { ...this.cache }
    for (const id in event.addedOrUpdated) copy[id] = event.addedOrUpdated[id]
    for (const id of event.removed) delete copy[id]

    this.notifyChanged(copy)
  }
}
