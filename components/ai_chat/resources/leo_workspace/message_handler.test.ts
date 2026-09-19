// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  installMessageHandler,
  kReadFileRequest,
  kReadFileResponse,
  viewOrigin,
} from './message_handler'
import { createFakeWorkspace } from './test_file_system'

// The page's own origin, and the `view.` sibling that is allowed to talk to it.
const kWorkspaceOrigin = 'chrome-untrusted://a-uuid.leo-workspace'
const kSenderOrigin = 'chrome-untrusted://view.a-uuid.leo-workspace'

let resolveRoot: (handle: FileSystemDirectoryHandle) => void
let rejectRoot: (reason: Error) => void
let source: { postMessage: jest.Mock }
let listener: EventListener

/**
 * Installs the handler over a handle that arrives only when resolveRoot() or
 * rejectRoot() is called. The listener is captured so afterEach can remove it;
 * one is added per install, and a leaked listener would answer later tests.
 */
function install() {
  const { promise, resolve, reject } =
    Promise.withResolvers<FileSystemDirectoryHandle>()
  resolveRoot = resolve
  rejectRoot = reject
  promise.catch(() => {})

  const add = jest.spyOn(window, 'addEventListener')
  installMessageHandler(promise, kWorkspaceOrigin)
  listener = add.mock.calls.find(
    (call) => call[0] === 'message',
  )![1] as EventListener
  add.mockRestore()
}

/** Resolves the handle with a small default workspace. */
function ready() {
  resolveRoot(
    createFakeWorkspace({
      'notes.txt': 'hello\nworld',
      'src/main.ts': 'export {}',
      // 'é' is two bytes in UTF-8, so a byte-accurate read reports size 3 where
      // a string-based one would report 2 characters.
      'bin.dat': 'aé',
    }),
  )
}

/** Dispatches a message as if it came from |origin| without waiting. */
function post(data: unknown, origin = kSenderOrigin) {
  window.dispatchEvent(
    new MessageEvent('message', { data, origin, source: source as never }),
  )
}

/** Lets pending promises settle, then returns the reply, if any. */
async function reply(): Promise<Record<string, unknown> | undefined> {
  await new Promise((resolve) => setTimeout(resolve, 0))
  expect(source.postMessage.mock.calls.length).toBeLessThan(2)
  const call = source.postMessage.mock.calls[0]
  if (!call) {
    return undefined
  }
  // The reply must be aimed at the trusted origin, never '*'.
  expect(call[1]).toBe(kSenderOrigin)
  return call[0]
}

/** Sends a message to a ready workspace and returns the reply. */
async function send(data: unknown, origin = kSenderOrigin) {
  ready()
  post(data, origin)
  return reply()
}

beforeEach(() => {
  source = { postMessage: jest.fn() }
  jest.spyOn(console, 'error').mockImplementation(() => {})
  install()
})

afterEach(() => {
  window.removeEventListener('message', listener)
})

describe('installMessageHandler', () => {
  it('installs no listener when no viewer origin can be derived', () => {
    const add = jest.spyOn(window, 'addEventListener')
    installMessageHandler(Promise.resolve(createFakeWorkspace({})), 'null')
    expect(add.mock.calls.filter((call) => call[0] === 'message')).toHaveLength(
      0,
    )
    add.mockRestore()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('no viewer origin'),
    )
  })
})

describe('viewOrigin', () => {
  it('prefixes the host of the workspace origin', () => {
    expect(viewOrigin(kWorkspaceOrigin)).toBe(kSenderOrigin)
  })

  it.each([
    ['null'],
    [''],
    ['not a url'],
    ['https://leo-workspace'],
    ['http://leo-workspace'],
    ['chrome://leo-workspace'],
  ])('returns empty for %p, which must not be trusted', (origin) => {
    expect(viewOrigin(origin)).toBe('')
  })
})

describe('READ_FILE handler', () => {
  it('replies with the file as a Blob', async () => {
    const response = await send({
      type: kReadFileRequest,
      path: 'notes.txt',
      requestId: 7,
    })
    expect(response).toMatchObject({
      type: kReadFileResponse,
      requestId: 7,
    })
    const file = response!.file as File
    expect(file.name).toBe('notes.txt')
    expect(await file.text()).toBe('hello\nworld')
  })

  it('reads a nested path', async () => {
    const response = await send({ type: kReadFileRequest, path: 'src/main.ts' })
    expect(await (response!.file as File).text()).toBe('export {}')
  })

  it('exposes bytes, not decoded text', async () => {
    const response = await send({ type: kReadFileRequest, path: 'bin.dat' })
    const file = response!.file as File
    expect(file.size).toBe(3)
    expect(new Uint8Array(await file.arrayBuffer())).toEqual(
      new Uint8Array([0x61, 0xc3, 0xa9]),
    )
  })

  it('serves a request that arrives before the handle does', async () => {
    post({ type: kReadFileRequest, path: 'notes.txt' })
    expect(await reply()).toBeUndefined()
    ready()
    expect(await ((await reply())!.file as File).text()).toBe('hello\nworld')
  })

  it('errors when the launch never delivers a handle', async () => {
    post({ type: kReadFileRequest, path: 'notes.txt' })
    rejectRoot(new Error('workspace folder is unavailable'))
    expect(await reply()).toMatchObject({
      error: 'workspace folder is unavailable',
    })
  })

  it('rejects a bad path without waiting for the handle', async () => {
    post({ type: kReadFileRequest, path: '' })
    expect(await reply()).toMatchObject({
      error: expect.stringContaining('requires a non-empty string "path"'),
    })
  })

  it.each([[undefined], ['' as unknown], [42], [null], [{}]])(
    'errors when path is %p',
    async (path) => {
      expect(await send({ type: kReadFileRequest, path })).toMatchObject({
        error: expect.stringContaining('requires a non-empty string "path"'),
      })
    },
  )

  it('errors for a path that escapes the workspace root', async () => {
    expect(
      await send({ type: kReadFileRequest, path: '../outside.txt' }),
    ).toMatchObject({ error: expect.stringContaining('escapes the workspace') })
  })

  it('errors for a file that does not exist', async () => {
    expect(await send({ type: kReadFileRequest, path: 'nope.txt' })).toEqual({
      type: kReadFileResponse,
      requestId: undefined,
      error: expect.any(String),
    })
  })

  it('errors for a directory', async () => {
    expect(await send({ type: kReadFileRequest, path: 'src' })).toMatchObject({
      error: expect.any(String),
    })
  })

  it.each([
    [kWorkspaceOrigin, 'the workspace origin itself'],
    ['chrome-untrusted://view.b-uuid.leo-workspace', 'another workspace'],
    ['chrome-untrusted://evil.a-uuid.leo-workspace', 'another subdomain'],
    ['https://view.a-uuid.leo-workspace', 'a different scheme'],
    ['chrome-untrusted://view.a-uuid.leo-workspace.evil', 'a longer host'],
  ])('ignores messages from %s (%s)', async (origin) => {
    expect(
      await send({ type: kReadFileRequest, path: 'notes.txt' }, origin),
    ).toBeUndefined()
  })

  it.each([
    [{ type: 'WRITE_FILE', path: 'notes.txt' }],
    [{ path: 'notes.txt' }],
    ['READ_FILE'],
    [null],
    [undefined],
  ])('ignores unrecognised payload %p', async (data) => {
    expect(await send(data)).toBeUndefined()
  })

  it('does not throw when there is no source to reply to', async () => {
    ready()
    window.dispatchEvent(
      new MessageEvent('message', {
        data: { type: kReadFileRequest, path: 'notes.txt' },
        origin: kSenderOrigin,
      }),
    )
    await new Promise((resolve) => setTimeout(resolve, 0))
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('no source to reply to'),
    )
  })
})
