// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Where a workspace file is served from on the viewer origin, and the folder it
// is read out of. Both halves of the viewer - the page that asks for a file
// (view.tsx) and the worker that serves it (sw.ts) - agree here.

// The URL path under the viewer origin that files are served from.
export const kFilesPrefix = '/files/'

// The URL |path| is served from. Segments are encoded one by one, so that '#',
// '?' and spaces in a name survive while the separators stay separators.
export function fileURL(path: string): string {
  return kFilesPrefix + path.split('/').map(encodeURIComponent).join('/')
}

// The workspace-relative path |url| asks for, or null for a URL that asks for
// no file. Percent-encoding is undone; a malformed encoding yields null rather
// than a partially decoded path.
export function filePathFromURL(url: URL): string | null {
  if (!url.pathname.startsWith(kFilesPrefix)) {
    return null
  }
  try {
    const path = decodeURIComponent(url.pathname.slice(kFilesPrefix.length))
    return path === '' ? null : path
  } catch {
    return null
  }
}

// Where the folder handle is kept. The browser delivers it to the viewer page
// (launchQueue), but the worker is what reads files with it, and a worker is
// stopped and restarted at the browser's discretion - so the handle is left in
// storage, which both halves can reach, rather than in the worker's memory.
const kDatabaseName = 'leo-workspace-view'
const kStoreName = 'folder'
const kFolderKey = 'root'

// The slice of storage the two halves share, so that tests can stand in for it.
export interface FolderStore {
  save(folder: FileSystemDirectoryHandle): Promise<void>
  load(): Promise<FileSystemDirectoryHandle | null>
}

function request<T>(request: IDBRequest<T>): Promise<T> {
  return new Promise((resolve, reject) => {
    request.onsuccess = () => resolve(request.result)
    request.onerror = () => reject(request.error)
  })
}

function openDatabase(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const open = indexedDB.open(kDatabaseName, 1)
    open.onupgradeneeded = () => open.result.createObjectStore(kStoreName)
    open.onsuccess = () => resolve(open.result)
    open.onerror = () => reject(open.error)
  })
}

async function withStore<T>(
  mode: IDBTransactionMode,
  use: (store: IDBObjectStore) => IDBRequest<T>,
): Promise<T> {
  const database = await openDatabase()
  try {
    return await request(
      use(database.transaction(kStoreName, mode).objectStore(kStoreName)),
    )
  } finally {
    database.close()
  }
}

// A FileSystemDirectoryHandle is structured-cloneable, so it survives being put
// in IndexedDB; what it grants is decided by the origin's permission, which the
// browser granted read-only when it handed the handle over.
export const indexedDbFolderStore: FolderStore = {
  async save(folder) {
    await withStore('readwrite', (store) => store.put(folder, kFolderKey))
  },
  async load() {
    const folder = await withStore<FileSystemDirectoryHandle | undefined>(
      'readonly',
      (store) => store.get(kFolderKey),
    )
    return folder ?? null
  },
}
