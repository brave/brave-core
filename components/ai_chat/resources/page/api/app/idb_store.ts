// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Minimal async key/value store backed by IndexedDB.

const DB_NAME = 'ai-chat-app'
const STORE_NAME = 'kv'
const DB_VERSION = 1

let dbPromise: Promise<IDBDatabase> | undefined

function openDB(): Promise<IDBDatabase> {
  if (dbPromise) return dbPromise
  dbPromise = new Promise((resolve, reject) => {
    const req = indexedDB.open(DB_NAME, DB_VERSION)
    req.onupgradeneeded = () => {
      req.result.createObjectStore(STORE_NAME)
    }
    req.onsuccess = () => resolve(req.result)
    req.onerror = () => reject(req.error)
  })
  return dbPromise
}

function tx(
  db: IDBDatabase,
  mode: IDBTransactionMode,
): IDBObjectStore {
  return db.transaction(STORE_NAME, mode).objectStore(STORE_NAME)
}

function wrap<T>(req: IDBRequest<T>): Promise<T> {
  return new Promise((resolve, reject) => {
    req.onsuccess = () => resolve(req.result)
    req.onerror = () => reject(req.error)
  })
}

export async function get<T>(key: string): Promise<T | undefined> {
  return wrap<T>((tx(await openDB(), 'readonly')).get(key))
}

export async function set<T>(key: string, value: T): Promise<void> {
  await wrap((tx(await openDB(), 'readwrite')).put(value, key))
}

export async function del(key: string): Promise<void> {
  await wrap((tx(await openDB(), 'readwrite')).delete(key))
}

export async function keys(): Promise<string[]> {
  return wrap<string[]>((tx(await openDB(), 'readonly')).getAllKeys() as IDBRequest<string[]>)
}
