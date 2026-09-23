// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { runInNewContext } from 'node:vm'
import {
  forgetFolderForTesting,
  kFileResponseHeaders,
  serveFile,
} from './viewer_file_server'
import { createFakeWorkspace, makeFileUnreadable } from './test_file_system'
import type { FolderStore } from './viewer_files'

// The shared test setup replaces the global decodeURIComponent with a stub that
// always returns 'test' (components/test/testPolyfills.ts), and a served path is
// exactly a percent-decoded one. Each test file gets a fresh jsdom environment,
// so this stays local to this file.
global.decodeURIComponent = runInNewContext('decodeURIComponent')

const kViewerOrigin = 'chrome-untrusted://view.a-uuid.leo-workspace'

/** A store holding |folder|, or holding nothing. */
function storeOf(folder: FileSystemDirectoryHandle | null): FolderStore {
  return { save: async () => {}, load: async () => folder }
}

function url(path: string): URL {
  return new URL(`${kViewerOrigin}${path}`)
}

beforeEach(() => {
  forgetFolderForTesting()
})

describe('serveFile', () => {
  it('serves a file out of the folder', async () => {
    const store = storeOf(createFakeWorkspace({ 'notes.txt': 'hello' }))

    const served = (await serveFile(url('/files/notes.txt'), store))!

    expect(served.status).toBe(200)
    expect(await (served.body as File).text()).toBe('hello')
  })

  it('serves a nested file, and one whose name needed encoding', async () => {
    const store = storeOf(
      createFakeWorkspace({ 'src/main.ts': 'export {}', 'a b.txt': 'spaced' }),
    )

    expect(
      await (
        (await serveFile(url('/files/src/main.ts'), store))!.body as File
      ).text(),
    ).toBe('export {}')
    expect(
      await (
        (await serveFile(url('/files/a%20b.txt'), store))!.body as File
      ).text(),
    ).toBe('spaced')
  })

  it('serves the media type the file has, or an opaque one', async () => {
    const store = storeOf(createFakeWorkspace({ 'notes.txt': '' }))
    // The fake reports no type, as the platform does for an unknown extension.
    expect((await serveFile(url('/files/notes.txt'), store))!.contentType).toBe(
      'application/octet-stream',
    )
  })

  it.each([
    ['/files', 'no trailing slash'],
    ['/files/', 'an empty path'],
    ['/leo_workspace_view.bundle.js', 'the viewer bundle'],
    ['/', 'the viewer page'],
  ])('declines %s (%s), which is not a file request', async (path) => {
    expect(
      await serveFile(url(path), storeOf(createFakeWorkspace())),
    ).toBeNull()
  })

  it('reports having no folder to read from', async () => {
    const served = (await serveFile(url('/files/notes.txt'), storeOf(null)))!

    // The viewer page stores the folder before it asks for a file, so this is a
    // request that arrived without one having been shown.
    expect(served.status).toBe(503)
    expect(served.body).toContain('no workspace folder')
  })

  it('reads the folder once, and keeps it for what follows', async () => {
    const load = jest.fn(async () =>
      createFakeWorkspace({ 'a.txt': 'a', 'b.txt': 'b' }),
    )
    const store: FolderStore = { save: async () => {}, load }

    await serveFile(url('/files/a.txt'), store)
    await serveFile(url('/files/b.txt'), store)

    // A document is followed by requests for whatever it references.
    expect(load).toHaveBeenCalledTimes(1)
  })

  it('reports a file that is not there as missing', async () => {
    const store = storeOf(createFakeWorkspace({ 'notes.txt': '' }))

    const served = (await serveFile(url('/files/nope.txt'), store))!

    expect(served.status).toBe(404)
    expect(served.body).toContain('could not read nope.txt')
  })

  it('reports a directory as missing rather than serving it', async () => {
    const store = storeOf(createFakeWorkspace({ 'src/main.ts': '' }))

    expect((await serveFile(url('/files/src'), store))!.status).toBe(404)
  })

  it.each([['/files/../outside.txt'], ['/files/%2E%2E/outside.txt']])(
    'is never asked for %s, which the URL parser walks out of /files/',
    async (path) => {
      expect(url(path).pathname).toBe('/outside.txt')
      expect(
        await serveFile(url(path), storeOf(createFakeWorkspace())),
      ).toBeNull()
    },
  )

  it('refuses an encoded path that would leave the folder', async () => {
    const store = storeOf(createFakeWorkspace({ 'notes.txt': '' }))

    // An encoded separator survives URL parsing, so '..' can still arrive as a
    // path segment; file_ops refuses it before the platform sees it.
    const served = (await serveFile(url('/files/..%2Foutside.txt'), store))!

    // Reported as missing rather than as forbidden: what is outside the folder
    // is not this origin's business to know about.
    expect(served.status).toBe(404)
    expect(served.body).toContain('escapes the workspace')
  })

  it('reports a file it is not allowed to read as forbidden', async () => {
    const folder = createFakeWorkspace({ 'secret.txt': '' })
    makeFileUnreadable(
      folder,
      'secret.txt',
      new DOMException('no', 'NotAllowedError'),
    )

    const served = (await serveFile(url('/files/secret.txt'), storeOf(folder)))!

    expect(served.status).toBe(403)
  })
})

describe('kFileResponseHeaders', () => {
  const csp = kFileResponseHeaders['content-security-policy']

  it('keeps a served file a client of the worker', () => {
    // Without allow-same-origin the document has an opaque origin, which no
    // worker controls, so nothing it asks for could be served.
    expect(csp).toContain('sandbox allow-same-origin allow-scripts')
  })

  it('leaves a served file no way to carry bytes out', () => {
    expect(csp).toContain("default-src 'self'")
    for (const directive of [
      'script-src',
      'style-src',
      'img-src',
      'media-src',
      'font-src',
      'frame-src',
    ]) {
      const value = csp.split('; ').find((d) => d.startsWith(directive))!
      expect(value).toContain("'self'")
      expect(value).not.toMatch(/https?:|\*/)
    }
    // Nothing to submit to, and no window to open: sandbox withholds
    // allow-popups, allow-forms and allow-top-navigation.
    expect(csp).toContain("form-action 'none'")
    expect(csp).not.toContain('allow-popups')
    expect(csp).not.toContain('allow-forms')
    expect(csp).not.toContain('allow-top-navigation')
  })

  it('serves a file as the type it was read as', () => {
    expect(kFileResponseHeaders['x-content-type-options']).toBe('nosniff')
  })
})
