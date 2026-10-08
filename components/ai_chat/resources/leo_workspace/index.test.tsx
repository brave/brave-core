// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createFakeWorkspace } from './test_file_system'

jest.mock('./storage', () => ({
  restoreDirectoryHandle: jest.fn().mockResolvedValue(null),
  storeDirectoryHandle: jest.fn().mockResolvedValue(undefined),
}))

interface LaunchParams {
  files: FileSystemHandle[]
}

interface RegisteredTool {
  name: string
  execute: (input: Record<string, unknown>) => Promise<unknown>
}

let consumer: ((params: LaunchParams) => void) | null
let setConsumer: jest.Mock<void, [(params: LaunchParams) => void]>
let registered: Map<string, RegisteredTool>
let getDirectory: jest.Mock<Promise<FileSystemDirectoryHandle>, []>
let storage: {
  restoreDirectoryHandle: jest.Mock
  storeDirectoryHandle: jest.Mock
}

/**
 * Loads a fresh copy of the entry point (it keeps the workspace root in module
 * state) and runs its DOMContentLoaded handler. jsdom has already finished
 * loading, so the handler is captured and invoked by hand rather than
 * dispatching the event, which would also re-run earlier tests' copies.
 */
async function load(
  configureStorage?: (s: typeof storage) => void,
): Promise<void> {
  const addEventListener = jest.spyOn(document, 'addEventListener')
  await jest.isolateModulesAsync(async () => {
    storage = (await import('./storage')) as unknown as typeof storage
    configureStorage?.(storage)
    await import('./index')
  })
  const call = addEventListener.mock.calls.find(
    ([type]) => type === 'DOMContentLoaded',
  )
  addEventListener.mockRestore()
  ;(call![1] as () => void)()
  await flush()
}

/** Lets floating promises settle. */
function flush() {
  return new Promise((resolve) => setTimeout(resolve, 0))
}

/** Runs a registered tool and returns its result as a string. */
async function run(name: string, input: Record<string, unknown>) {
  return String(await registered.get(name)!.execute(input))
}

const kViewTool = 'view'
const kEditTool = 'str_replace_based_edit_tool'
const view = (path: string) => run(kViewTool, { path })

/**
 * Errors logged by the launch, ignoring the notice that the viewer message
 * handler emits because jsdom serves the tests from http://localhost instead of
 * the page's chrome-untrusted origin.
 */
function launchErrors() {
  return (console.error as jest.Mock).mock.calls.filter(
    ([message]) => !String(message).includes('no viewer origin'),
  )
}

beforeEach(() => {
  consumer = null
  setConsumer = jest.fn((c: (params: LaunchParams) => void) => {
    consumer = c
  })
  Object.defineProperty(window, 'launchQueue', {
    configurable: true,
    writable: true,
    value: { setConsumer },
  })

  registered = new Map()
  Object.defineProperty(document, 'modelContext', {
    configurable: true,
    writable: true,
    value: {
      registerTool: async (tool: RegisteredTool) => {
        registered.set(tool.name, tool)
      },
    },
  })

  getDirectory = jest.fn(async () =>
    createFakeWorkspace({ 'opfs.txt': 'from opfs' }, 'opfs'),
  )
  Object.defineProperty(navigator, 'storage', {
    configurable: true,
    writable: true,
    value: { getDirectory },
  })

  jest.spyOn(console, 'log').mockImplementation(() => {})
  jest.spyOn(console, 'error').mockImplementation(() => {})
})

afterEach(() => {
  delete document.modelContext
  delete window.launchQueue
  delete (navigator as { storage?: unknown }).storage
  jest.restoreAllMocks()
})

describe('leo workspace entry point', () => {
  it('consumes the launch queue once the document is ready', async () => {
    await load()
    expect(setConsumer).toHaveBeenCalledTimes(1)
    expect(launchErrors()).toEqual([])
  })

  it('registers the file tools before a folder arrives', async () => {
    await load()
    expect([...registered.keys()]).toEqual([
      kViewTool,
      kEditTool,
      'grep',
      'glob',
      'append_file',
    ])
    expect(getDirectory).not.toHaveBeenCalled()
  })

  it('runs the tools against the delivered directory handle', async () => {
    await load()
    const root = createFakeWorkspace({ 'a.txt': 'hello' })
    consumer!({ files: [root] })
    expect(await view('a.txt')).toBe('1\thello')
    expect(storage.storeDirectoryHandle).toHaveBeenCalledWith(root)
    expect(getDirectory).not.toHaveBeenCalled()
    expect(launchErrors()).toEqual([])
  })

  it('runs the tools against the handle restored from IndexedDB', async () => {
    const root = createFakeWorkspace({ 'a.txt': 'stored' })
    await load((s) => s.restoreDirectoryHandle.mockResolvedValue(root))
    expect(setConsumer).not.toHaveBeenCalled()
    expect(await view('a.txt')).toBe('1\tstored')
    expect(storage.storeDirectoryHandle).not.toHaveBeenCalled()
    expect(getDirectory).not.toHaveBeenCalled()
  })

  it('falls back to OPFS when a tool runs without a folder', async () => {
    await load()
    expect(await view('opfs.txt')).toBe('1\tfrom opfs')
    expect(getDirectory).toHaveBeenCalledTimes(1)
    const opfs = await getDirectory.mock.results[0].value
    expect(storage.storeDirectoryHandle).toHaveBeenCalledWith(opfs)
  })

  it('stores OPFS once across concurrent tool calls', async () => {
    await load()
    await Promise.all([view('opfs.txt'), view('opfs.txt')])
    await view('opfs.txt')
    expect(storage.storeDirectoryHandle).toHaveBeenCalledTimes(1)
  })

  it('ignores a launch that arrives after OPFS was adopted', async () => {
    await load()
    await view('opfs.txt')
    consumer!({ files: [createFakeWorkspace({ 'a.txt': 'late' })] })
    expect(await view('opfs.txt')).toBe('1\tfrom opfs')
    expect(storage.storeDirectoryHandle).toHaveBeenCalledTimes(1)
  })

  it('retries OPFS after a failure', async () => {
    await load()
    getDirectory.mockRejectedValueOnce(new Error('opfs unavailable'))
    expect(await view('opfs.txt')).toBe('Error: opfs unavailable')
    expect(await view('opfs.txt')).toBe('1\tfrom opfs')
    expect(getDirectory).toHaveBeenCalledTimes(2)
  })

  it('ignores a launch that delivers a file instead of a directory', async () => {
    await load()
    consumer!({ files: [{ kind: 'file', name: 'a.txt' } as FileSystemHandle] })
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('launch params missing a directory handle'),
      expect.anything(),
    )
    // Still folder-less, so the tools fall back to OPFS.
    expect(await view('opfs.txt')).toBe('1\tfrom opfs')
  })

  it('ignores a launch with no files', async () => {
    await load()
    consumer!({ files: [] })
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('launch params missing a directory handle'),
      expect.anything(),
    )
  })

  it('falls back to OPFS when the launch queue is unavailable', async () => {
    delete window.launchQueue
    await load()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('window.launchQueue is unavailable'),
    )
    expect(await view('opfs.txt')).toBe('1\tfrom opfs')
  })
})
