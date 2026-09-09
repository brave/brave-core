// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { serveFiles } from './file_server'
import { createFakeWorkspace } from './test_file_system'

// Stands in for navigator.serviceWorker: keeps the listener so a test can play
// the part of the worker and post a request to the page.
class FakeContainer {
  listener: ((event: MessageEvent) => void) | null = null
  startMessages = jest.fn()

  addEventListener(type: string, listener: (event: MessageEvent) => void) {
    if (type === 'message') {
      this.listener = listener
    }
  }

  /** Sends |data| with a reply port, and resolves with what comes back. */
  request(data: unknown): Promise<any> {
    return new Promise((resolve) => {
      const port = { postMessage: (reply: unknown) => resolve(reply) }
      this.listener!({
        data,
        ports: [port],
      } as unknown as MessageEvent)
    })
  }
}

let container: FakeContainer

function serve(files: Record<string, string>) {
  serveFiles(createFakeWorkspace(files) as FileSystemDirectoryHandle)
}

beforeEach(() => {
  container = new FakeContainer()
  Object.defineProperty(navigator, 'serviceWorker', {
    configurable: true,
    writable: true,
    value: container,
  })
  jest.spyOn(console, 'warn').mockImplementation(() => {})
  jest.spyOn(console, 'error').mockImplementation(() => {})
})

afterEach(() => {
  // @ts-expect-error - reset the stand-in.
  delete navigator.serviceWorker
})

describe('workspace file server', () => {
  it('starts the queued messages the worker has already sent', () => {
    serve({ 'a.txt': 'hi' })
    expect(container.startMessages).toHaveBeenCalledTimes(1)
  })

  it('replies with the bytes of a file in the folder', async () => {
    serve({ 'docs/report.html': '<h1>hi</h1>' })
    const reply = await container.request({
      type: 'leo-workspace-read-file',
      path: 'docs/report.html',
    })
    expect(new TextDecoder().decode(reply.bytes)).toBe('<h1>hi</h1>')
  })

  it('replies without bytes for a file that is not there', async () => {
    serve({ 'a.txt': 'hi' })
    const reply = await container.request({
      type: 'leo-workspace-read-file',
      path: 'nope.txt',
    })
    expect(reply.bytes).toBeUndefined()
  })

  it('replies without bytes for a path that leaves the folder', async () => {
    serve({ 'a.txt': 'hi' })
    const reply = await container.request({
      type: 'leo-workspace-read-file',
      path: '../outside.txt',
    })
    expect(reply.bytes).toBeUndefined()
  })

  it('ignores a message that is not a file request', async () => {
    serve({ 'a.txt': 'hi' })
    const port = { postMessage: jest.fn() }
    container.listener!({
      data: { type: 'something-else' },
      ports: [port],
    } as unknown as MessageEvent)
    await new Promise((resolve) => setTimeout(resolve, 0))
    expect(port.postMessage).not.toHaveBeenCalled()
  })
})

describe('workspace file server size cap', () => {
  it('replies without bytes for a file too large to serve', async () => {
    serve({ 'huge.bin': 'x'.repeat(64 * 1024 * 1024 + 1) })
    const reply = await container.request({
      type: 'leo-workspace-read-file',
      path: 'huge.bin',
    })
    expect(reply.bytes).toBeUndefined()
  })
})
