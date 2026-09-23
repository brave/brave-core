// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createFakeWorkspace } from './test_file_system'

// Importing the entry point installs its DOMContentLoaded listener. jsdom has
// already finished loading by then, so the tests dispatch the event by hand.
import { showViewerFrame } from './index'

// The page's own origin, and the viewer it is allowed to frame.
const kWorkspaceOrigin = 'chrome-untrusted://a-uuid.leo-workspace'
const kViewerOrigin = 'chrome-untrusted://view.a-uuid.leo-workspace'

interface LaunchParams {
  files: FileSystemHandle[]
}

let consumer: ((params: LaunchParams) => void) | null
let setConsumer: jest.Mock<void, [(params: LaunchParams) => void]>
let registeredToolNames: string[]
let root: HTMLElement

/** Runs the module's DOMContentLoaded handler. */
function load() {
  document.dispatchEvent(new Event('DOMContentLoaded'))
}

/** Lets the floating registerTools() promise settle. */
function flush() {
  return new Promise((resolve) => setTimeout(resolve, 0))
}

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

  registeredToolNames = []
  Object.defineProperty(document, 'modelContext', {
    configurable: true,
    writable: true,
    value: {
      registerTool: async (tool: { name: string }) => {
        registeredToolNames.push(tool.name)
      },
    },
  })

  root = document.createElement('div')
  root.id = 'root'
  document.body.appendChild(root)

  jest.spyOn(console, 'log').mockImplementation(() => {})
  jest.spyOn(console, 'error').mockImplementation(() => {})
})

afterEach(() => {
  // The framed viewer is module state, so drop it before the next test.
  window.location.hash = ''
  showViewerFrame(root)
  root.remove()
  delete document.modelContext
  delete window.launchQueue
})

describe('leo workspace entry point', () => {
  it('consumes the launch queue once the document is ready', () => {
    load()
    expect(setConsumer).toHaveBeenCalledTimes(1)
    expect(launchErrors()).toEqual([])
  })

  it('registers the file tools for the delivered directory handle', async () => {
    load()
    consumer!({ files: [createFakeWorkspace({ 'a.txt': '' })] })
    await flush()
    expect(registeredToolNames).toEqual([
      'str_replace_based_edit_tool',
      'grep',
      'glob',
      'append_file',
    ])
    expect(launchErrors()).toEqual([])
  })

  it('ignores a launch that delivers a file instead of a directory', async () => {
    load()
    consumer!({ files: [{ kind: 'file', name: 'a.txt' } as FileSystemHandle] })
    await flush()
    expect(registeredToolNames).toEqual([])
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('launch params missing a directory handle'),
      expect.anything(),
    )
  })

  it('ignores a launch with no files', async () => {
    load()
    consumer!({ files: [] })
    await flush()
    expect(registeredToolNames).toEqual([])
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('launch params missing a directory handle'),
      expect.anything(),
    )
  })

  it('logs when the launch queue is unavailable', () => {
    delete window.launchQueue
    load()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('window.launchQueue is unavailable'),
    )
  })
})

describe('viewer frame', () => {
  it('frames the viewer, full page, for the file in the fragment', () => {
    window.location.hash = '#file=notes.txt'

    showViewerFrame(root, kWorkspaceOrigin)

    const frame = root.querySelector('iframe')!
    expect(frame.src).toBe(`${kViewerOrigin}/#file=notes.txt`)
    expect(frame.style.position).toBe('fixed')
    expect(frame.style.width).toBe('100%')
    expect(frame.style.height).toBe('100%')
    expect(launchErrors()).toEqual([])
  })

  it('frames nothing when the fragment names no file', () => {
    showViewerFrame(root, kWorkspaceOrigin)
    expect(root.querySelector('iframe')).toBeNull()
  })

  it('retargets the frame it already has, rather than replacing it', () => {
    window.location.hash = '#file=first.txt'
    showViewerFrame(root, kWorkspaceOrigin)
    const frame = root.querySelector('iframe')

    window.location.hash = '#file=second.txt'
    showViewerFrame(root, kWorkspaceOrigin)

    // The same element, so the framed document - and the worker controlling
    // it - survives a retarget.
    expect(root.querySelector('iframe')).toBe(frame)
    expect(frame!.src).toBe(`${kViewerOrigin}/#file=second.txt`)
    expect(root.childElementCount).toBe(1)
  })

  it('removes the frame when the fragment stops naming a file', () => {
    window.location.hash = '#file=notes.txt'
    showViewerFrame(root, kWorkspaceOrigin)

    window.location.hash = ''
    showViewerFrame(root, kWorkspaceOrigin)

    expect(root.querySelector('iframe')).toBeNull()
  })

  it('frames nothing for an origin that has no viewer', () => {
    window.location.hash = '#file=notes.txt'

    showViewerFrame(root, 'null')

    expect(root.querySelector('iframe')).toBeNull()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('no viewer origin to frame'),
    )
  })

  it('frames on load, and again when the fragment changes', () => {
    // jsdom serves the tests from http://localhost, which has no viewer
    // sibling, so the attempt is what is observable here.
    window.location.hash = '#file=notes.txt'
    load()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('no viewer origin to frame'),
    )
    ;(console.error as jest.Mock).mockClear()
    window.location.hash = '#file=other.txt'
    window.dispatchEvent(new Event('hashchange'))
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('no viewer origin to frame'),
    )
  })
})
