// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { fileForLaunch } from './view'
import { createFakeWorkspace } from './test_file_system'
import { indexedDbFolderStore } from './viewer_files'

interface LaunchParams {
  files: FileSystemHandle[]
}

let serviceWorkerListeners: Map<string, EventListener[]>
let consumer: ((params: LaunchParams) => void) | null
let saved: FileSystemDirectoryHandle | null

/**
 * Stands in for navigator.serviceWorker, capturing the listeners that are added
 * to it, and whether a worker is already controlling the page.
 */
function defineServiceWorker({ controlled }: { controlled: boolean }) {
  serviceWorkerListeners = new Map()
  Object.defineProperty(navigator, 'serviceWorker', {
    configurable: true,
    value: {
      addEventListener: jest.fn((type: string, listener: EventListener) => {
        serviceWorkerListeners.set(type, [
          ...(serviceWorkerListeners.get(type) ?? []),
          listener,
        ])
      }),
      controller: controlled ? {} : null,
    },
  })
}

/** Dispatches |type| to everything listening on the container. */
function fireServiceWorkerEvent(type: string) {
  for (const listener of serviceWorkerListeners.get(type) ?? []) {
    listener(new Event(type))
  }
}

/** Runs the module's DOMContentLoaded handler. */
function load() {
  document.dispatchEvent(new Event('DOMContentLoaded'))
}

/** Lets pending promises settle. */
function flush() {
  return new Promise((resolve) => setTimeout(resolve, 0))
}

/** Delivers |handle| the way the browser does, through launchQueue. */
function launch(handle: FileSystemHandle) {
  return fileForLaunch({ files: [handle] })
}

beforeEach(() => {
  jest.spyOn(console, 'error').mockImplementation(() => {})
  jest.spyOn(console, 'log').mockImplementation(() => {})

  consumer = null
  Object.defineProperty(window, 'launchQueue', {
    configurable: true,
    writable: true,
    value: {
      setConsumer: (c: (params: LaunchParams) => void) => {
        consumer = c
      },
    },
  })

  // jsdom has no IndexedDB, and what is being checked is that the folder is put
  // somewhere the worker reads from, not how.
  saved = null
  jest
    .spyOn(indexedDbFolderStore, 'save')
    .mockImplementation(async (folder) => {
      saved = folder
    })
})

afterEach(() => {
  window.location.hash = ''
  delete (navigator as { serviceWorker?: unknown }).serviceWorker
  delete window.launchQueue
})

describe('the viewer page', () => {
  it('consumes the launch queue once the document is ready', () => {
    defineServiceWorker({ controlled: true })

    load()

    expect(consumer).not.toBeNull()
    expect(console.error).not.toHaveBeenCalled()
  })

  it('reports a launch queue it cannot be launched through', () => {
    defineServiceWorker({ controlled: true })
    delete window.launchQueue

    load()

    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('window.launchQueue is unavailable'),
    )
  })
})

describe('fileForLaunch', () => {
  it('keeps the folder, then says where its file is served', async () => {
    defineServiceWorker({ controlled: true })
    window.location.hash = '#file=notes.txt'
    const folder = createFakeWorkspace({ 'notes.txt': '' })

    const url = await launch(folder)

    // Kept first: the worker must have something to read before it is asked.
    expect(saved).toBe(folder)
    expect(url).toBe('/files/notes.txt')
  })

  it('encodes the path it asks for', async () => {
    defineServiceWorker({ controlled: true })
    window.location.hash = '#file=my%20dir/a%23b.txt'

    expect(await launch(createFakeWorkspace())).toBe(
      '/files/my%20dir/a%23b.txt',
    )
  })

  it('keeps the folder even when no file is asked for', async () => {
    defineServiceWorker({ controlled: true })
    const folder = createFakeWorkspace()

    expect(await launch(folder)).toBeNull()
    expect(saved).toBe(folder)
  })

  it('ignores a launch that delivers no folder', async () => {
    defineServiceWorker({ controlled: true })
    window.location.hash = '#file=notes.txt'

    const url = await launch({
      kind: 'file',
      name: 'notes.txt',
    } as FileSystemHandle)

    expect(url).toBeNull()
    expect(saved).toBeNull()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('launch params missing a directory handle'),
      expect.anything(),
    )
  })

  it('shows nothing when the folder could not be kept', async () => {
    defineServiceWorker({ controlled: true })
    window.location.hash = '#file=notes.txt'
    jest
      .spyOn(indexedDbFolderStore, 'save')
      .mockRejectedValue(new Error('no storage'))

    expect(await launch(createFakeWorkspace())).toBeNull()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('could not keep the folder handle'),
      expect.anything(),
    )
  })

  it('waits for a worker to control the page', async () => {
    // Until one does, /files/ reaches the data source, which answers an
    // unmatched path with this document - which would navigate again.
    defineServiceWorker({ controlled: false })
    window.location.hash = '#file=notes.txt'

    let resolved: string | null | undefined
    const asked = launch(createFakeWorkspace()).then((url) => (resolved = url))
    await flush()
    expect(resolved).toBeUndefined()

    fireServiceWorkerEvent('controllerchange')
    await asked
    expect(resolved).toBe('/files/notes.txt')
  })

  it('shows nothing when no worker can control the page', async () => {
    window.location.hash = '#file=notes.txt'

    expect(await launch(createFakeWorkspace())).toBeNull()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('navigator.serviceWorker is unavailable'),
    )
  })
})
