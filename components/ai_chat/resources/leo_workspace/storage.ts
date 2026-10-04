// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// IndexedDB storage for persisting the workspace's FileSystemDirectoryHandle.
// This allows the workspace to restore its folder handle across page reloads
// without requiring launchQueue to deliver it again.

// Extend FileSystemHandle to include requestPermission (not in default TS lib)
interface FileSystemHandlePermissionDescriptor {
  mode?: 'read' | 'readwrite'
}

declare global {
  interface FileSystemHandle {
    requestPermission(
      descriptor?: FileSystemHandlePermissionDescriptor,
    ): Promise<PermissionState>
  }
}

const kDbName = 'leo-workspace'
const kDbVersion = 1
const kStoreName = 'handles'
const kHandleKey = 'root'

function openDb(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(kDbName, kDbVersion)

    request.onerror = () => {
      reject(new Error(`Failed to open IndexedDB: ${request.error?.message}`))
    }

    request.onsuccess = () => {
      resolve(request.result)
    }

    request.onupgradeneeded = (event) => {
      const db = (event.target as IDBOpenDBRequest).result
      if (!db.objectStoreNames.contains(kStoreName)) {
        db.createObjectStore(kStoreName)
      }
    }
  })
}

// Stores the directory handle in IndexedDB.
export async function storeDirectoryHandle(
  handle: FileSystemDirectoryHandle,
): Promise<void> {
  const db = await openDb()
  return new Promise((resolve, reject) => {
    const transaction = db.transaction(kStoreName, 'readwrite')
    const store = transaction.objectStore(kStoreName)
    const request = store.put(handle, kHandleKey)

    request.onerror = () => {
      reject(new Error(`Failed to store handle: ${request.error?.message}`))
    }

    request.onsuccess = () => {
      console.log('[leo-workspace] directory handle stored in IndexedDB')
      resolve()
    }

    transaction.oncomplete = () => {
      db.close()
    }
  })
}

// Retrieves the directory handle from IndexedDB and requests permission.
// Returns null if no handle is stored or permission is denied.
export async function restoreDirectoryHandle(): Promise<FileSystemDirectoryHandle | null> {
  try {
    const db = await openDb()
    const handle = await new Promise<FileSystemDirectoryHandle | null>(
      (resolve, reject) => {
        const transaction = db.transaction(kStoreName, 'readonly')
        const store = transaction.objectStore(kStoreName)
        const request = store.get(kHandleKey)

        request.onerror = () => {
          reject(new Error(`Failed to get handle: ${request.error?.message}`))
        }

        request.onsuccess = () => {
          resolve(request.result ?? null)
        }

        transaction.oncomplete = () => {
          db.close()
        }
      },
    )

    if (!handle) {
      console.log('[leo-workspace] no stored directory handle found')
      return null
    }

    // Request permission on the restored handle
    const permission = await handle.requestPermission({ mode: 'read' })
    if (permission !== 'granted') {
      console.log('[leo-workspace] permission denied for stored handle')
      return null
    }

    console.log(
      '[leo-workspace] restored directory handle from IndexedDB:',
      handle.name,
    )
    return handle
  } catch (e) {
    console.error('[leo-workspace] failed to restore directory handle:', e)
    return null
  }
}
