// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { documentResponseStatus, filePathToUrlPath } from './leo_workspace_view'
import { kFileDisplayed, kFileDisplayFailed } from './message_handler'

describe('filePathToUrlPath', () => {
  it('keeps the path structure', () => {
    expect(filePathToUrlPath('docs/page.html')).toBe('/docs/page.html')
  })

  it('escapes each segment, so the service worker sees the real name', () => {
    const urlPath = filePathToUrlPath('my docs/a b#1?.html')
    expect(urlPath).toBe('/my%20docs/a%20b%231%3F.html')
    // The URL keeps the whole name in its path, as the service worker reads
    // it, rather than splitting it off into a fragment or query. (Parsed
    // against an https base, as jsdom doesn't know chrome-untrusted is a
    // standard scheme.)
    const url = new URL(urlPath, 'https://view.abc.leo-workspace')
    expect(url.hash).toBe('')
    expect(url.search).toBe('')
    // Not compared after decodeURIComponent(), which the jest polyfills stub
    // out (see components/test/testPolyfills.ts).
    expect(url.pathname).toBe(urlPath)
  })
})

/** Makes |iframe|'s document look as if it was served with |status|. */
function setResponseStatus(iframe: HTMLIFrameElement, status: number) {
  Object.defineProperty(iframe, 'contentWindow', {
    configurable: true,
    value: {
      performance: {
        getEntriesByType: (type: string) =>
          type === 'navigation' ? [{ responseStatus: status }] : [],
      },
    },
  })
}

describe('documentResponseStatus', () => {
  it('is the status the document was served with', () => {
    const iframe = document.createElement('iframe')
    setResponseStatus(iframe, 404)
    expect(documentResponseStatus(iframe)).toBe(404)
  })

  it('is unknown when the status is not exposed', () => {
    const iframe = document.createElement('iframe')
    setResponseStatus(iframe, 0)
    expect(documentResponseStatus(iframe)).toBeNull()
  })

  it('is unknown for a document the viewer cannot read', () => {
    const iframe = document.createElement('iframe')
    Object.defineProperty(iframe, 'contentWindow', {
      value: {
        get performance() {
          throw new DOMException('Blocked', 'SecurityError')
        },
      },
    })
    expect(documentResponseStatus(iframe)).toBeNull()
  })
})

describe('the viewer', () => {
  const kViewerOrigin = 'chrome-untrusted://view.abc.leo-workspace'
  const kWorkspaceOrigin = 'chrome-untrusted://abc.leo-workspace'

  let parent: { postMessage: jest.Mock }
  let initialize: (viewerOrigin: string, parent: Window | null) => Promise<void>

  /**
   * Stands in for the viewer's service worker: |active| is whether there is
   * one, which answers the viewer's INIT with READY.
   */
  function setServiceWorker(active: boolean) {
    // The viewer's end of the channel it gives the service worker.
    const port: {
      postMessage: jest.Mock
      onmessage: ((event: { data: unknown }) => void) | null
    } = { postMessage: jest.fn(), onmessage: null }
    class FakeMessageChannel {
      port1 = port
      port2 = {}
    }
    Object.defineProperty(globalThis, 'MessageChannel', {
      configurable: true,
      value: FakeMessageChannel,
    })
    Object.defineProperty(navigator, 'serviceWorker', {
      configurable: true,
      value: {
        ready: Promise.resolve({
          active: active
            ? {
                postMessage: () =>
                  port.onmessage?.({ data: { type: 'READY' } }),
              }
            : null,
        }),
      },
    })
  }

  beforeEach(async () => {
    jest.spyOn(console, 'debug').mockImplementation(() => {})
    document.body.innerHTML = '<div id="root"></div>'
    window.history.replaceState(null, '', '#file=docs/page.html')
    parent = { postMessage: jest.fn() }
    // A fresh copy, as the viewer keeps its state in the module.
    await jest.isolateModulesAsync(async () => {
      ;({ initialize } = await import('./leo_workspace_view'))
    })
  })

  afterEach(() => {
    jest.restoreAllMocks()
    window.history.replaceState(null, '', '#')
  })

  /** Loads the displayed file's frame, as if it was served with |status|. */
  function loadFile(status: number) {
    const iframe = document.querySelector<HTMLIFrameElement>('#root iframe')
    expect(iframe).not.toBeNull()
    expect(iframe!.getAttribute('src')).toBe('/docs/page.html')
    setResponseStatus(iframe!, status)
    parent.postMessage.mockClear()
    iframe!.dispatchEvent(new Event('load'))
  }

  it('tells the workspace once the file has loaded', async () => {
    setServiceWorker(true)
    await initialize(kViewerOrigin, parent as unknown as Window)

    loadFile(200)
    expect(parent.postMessage).toHaveBeenCalledTimes(1)
    expect(parent.postMessage).toHaveBeenCalledWith(
      { type: kFileDisplayed, file: 'docs/page.html' },
      kWorkspaceOrigin,
    )
  })

  it('tells the workspace when the file could not be loaded', async () => {
    setServiceWorker(true)
    await initialize(kViewerOrigin, parent as unknown as Window)

    // The service worker's answer for a file that doesn't exist.
    loadFile(404)
    expect(parent.postMessage).toHaveBeenCalledTimes(1)
    expect(parent.postMessage).toHaveBeenCalledWith(
      {
        type: kFileDisplayFailed,
        file: 'docs/page.html',
        error: 'The file could not be loaded (HTTP 404)',
      },
      kWorkspaceOrigin,
    )
  })

  it('tells the workspace when the file could not be displayed', async () => {
    jest.spyOn(console, 'error').mockImplementation(() => {})
    setServiceWorker(false)
    await initialize(kViewerOrigin, parent as unknown as Window)

    expect(document.querySelector('#root iframe')).toBeNull()
    expect(parent.postMessage).toHaveBeenCalledTimes(1)
    expect(parent.postMessage).toHaveBeenCalledWith(
      {
        type: kFileDisplayFailed,
        file: 'docs/page.html',
        error: 'No active service worker',
      },
      kWorkspaceOrigin,
    )
  })
})
