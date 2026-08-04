// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createFakeWorkspace } from './test_file_system'

// Importing the entry point installs its DOMContentLoaded listener. jsdom has
// already finished loading by then, so the tests dispatch the event by hand.
import './index'

interface LaunchParams {
  files: FileSystemHandle[]
}

let consumer: ((params: LaunchParams) => void) | null
let setConsumer: jest.Mock<void, [(params: LaunchParams) => void]>
let registeredToolNames: string[]

/** Runs the module's DOMContentLoaded handler. */
function load() {
  document.dispatchEvent(new Event('DOMContentLoaded'))
}

/** Lets the floating registerTools() promise settle. */
function flush() {
  return new Promise((resolve) => setTimeout(resolve, 0))
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
  Object.defineProperty(navigator, 'modelContext', {
    configurable: true,
    writable: true,
    value: {
      registerTool: async (tool: { name: string }) => {
        registeredToolNames.push(tool.name)
      },
    },
  })

  jest.spyOn(console, 'log').mockImplementation(() => {})
  jest.spyOn(console, 'error').mockImplementation(() => {})
})

afterEach(() => {
  delete navigator.modelContext
  delete window.launchQueue
})

describe('leo workspace entry point', () => {
  it('consumes the launch queue once the document is ready', () => {
    load()
    expect(setConsumer).toHaveBeenCalledTimes(1)
    expect(console.error).not.toHaveBeenCalled()
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
    expect(console.error).not.toHaveBeenCalled()
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
