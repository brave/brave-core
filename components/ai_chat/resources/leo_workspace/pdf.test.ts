// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { kFileDisplayed, kFileDisplayFailed } from './message_handler'
import { createPdfPrinter, readBigBuffer } from './pdf'

// The real bindings need Mojo, which jsdom doesn't have. Every test passes its
// own remote.
jest.mock(
  'gen/brave/components/ai_chat/core/common/mojom/leo_workspace.mojom.m.js',
  () => ({ LeoWorkspaceHandler: { getRemote: jest.fn() } }),
)

const kOrigin = 'chrome-untrusted://abc.leo-workspace'
const kViewerOrigin = 'chrome-untrusted://view.abc.leo-workspace'
const kPdf = [0x25, 0x50, 0x44, 0x46]

let printFileToPdf: jest.Mock

beforeEach(() => {
  printFileToPdf = jest.fn(async () => ({ bytes: kPdf }))
})

afterEach(() => {
  jest.useRealTimers()
  document.body.replaceChildren()
})

function print(path: string, landscape = false) {
  return createPdfPrinter(kOrigin, () => ({ printFileToPdf }))(path, {
    landscape,
  })
}

function viewerFrame(): HTMLIFrameElement {
  const frames = document.body.querySelectorAll('iframe')
  expect(frames).toHaveLength(1)
  return frames[0]
}

/** Posts |data| to the workspace as if from |frame|'s viewer. */
function postFromViewer(
  data: unknown,
  { frame = viewerFrame(), origin = kViewerOrigin } = {},
) {
  window.dispatchEvent(
    new MessageEvent('message', {
      data,
      origin,
      source: frame.contentWindow,
    }),
  )
}

/** Lets pending promise callbacks run. */
const flush = () => new Promise((resolve) => setTimeout(resolve, 0))

describe('createPdfPrinter', () => {
  it('displays the file in a named viewer frame, then prints that frame', async () => {
    const result = print('docs/a b.html', true)

    const frame = viewerFrame()
    expect(frame.src).toBe(`${kViewerOrigin}/#file=docs%2Fa%20b.html`)
    expect(frame.name).toMatch(/^leo-workspace-pdf-/)
    // Laid out, but out of sight.
    expect(frame.style.position).toBe('fixed')
    expect(parseInt(frame.style.left)).toBeLessThan(
      -parseInt(frame.style.width),
    )
    // Nothing is printed before the viewer has displayed the file.
    await flush()
    expect(printFileToPdf).not.toHaveBeenCalled()

    postFromViewer({ type: kFileDisplayed, file: 'docs/a b.html' })
    expect(await result).toEqual(new Uint8Array(kPdf))
    expect(printFileToPdf).toHaveBeenCalledWith(frame.name, true)
    expect(document.body.querySelector('iframe')).toBeNull()
  })

  it('gives each print its own frame', async () => {
    const first = print('a.html')
    const second = print('a.html')
    const [frameA, frameB] = document.body.querySelectorAll('iframe')
    expect(frameA.name).not.toBe(frameB.name)

    postFromViewer({ type: kFileDisplayed, file: 'a.html' }, { frame: frameB })
    await second
    expect(printFileToPdf).toHaveBeenCalledTimes(1)
    expect(printFileToPdf).toHaveBeenCalledWith(frameB.name, false)

    postFromViewer({ type: kFileDisplayed, file: 'a.html' }, { frame: frameA })
    await first
    expect(printFileToPdf).toHaveBeenLastCalledWith(frameA.name, false)
  })

  it('ignores messages that are not from its viewer about its file', async () => {
    const result = print('a.html')
    const frame = viewerFrame()
    const other = document.createElement('iframe')
    document.body.appendChild(other)

    postFromViewer(
      { type: kFileDisplayed, file: 'a.html' },
      { frame, origin: kOrigin },
    )
    postFromViewer({ type: kFileDisplayed, file: 'a.html' }, { frame: other })
    postFromViewer({ type: kFileDisplayed, file: 'b.html' }, { frame })
    postFromViewer({ type: 'READ_FILE', file: 'a.html' }, { frame })
    postFromViewer(null, { frame })
    await flush()
    expect(printFileToPdf).not.toHaveBeenCalled()

    postFromViewer({ type: kFileDisplayed, file: 'a.html' }, { frame })
    await result
    expect(printFileToPdf).toHaveBeenCalledTimes(1)
  })

  it('fails when the viewer cannot display the file', async () => {
    const result = print('a.html')
    postFromViewer({
      type: kFileDisplayFailed,
      file: 'a.html',
      error: 'no service worker',
    })
    await expect(result).rejects.toThrow(
      'could not render a.html: no service worker',
    )
    expect(printFileToPdf).not.toHaveBeenCalled()
    expect(document.body.querySelector('iframe')).toBeNull()
  })

  it('times out when the viewer never displays the file', async () => {
    jest.useFakeTimers()
    const result = print('a.html')
    jest.advanceTimersByTime(30000)
    await expect(result).rejects.toThrow('timed out rendering a.html')
    expect(printFileToPdf).not.toHaveBeenCalled()
    expect(document.body.querySelector('iframe')).toBeNull()
  })

  it('reports the browser failing to print', async () => {
    printFileToPdf.mockRejectedValue('Print failed')
    const result = print('a.html')
    postFromViewer({ type: kFileDisplayed, file: 'a.html' })
    await expect(result).rejects.toThrow(
      'could not print a.html to PDF: Print failed',
    )
    expect(document.body.querySelector('iframe')).toBeNull()
  })

  it('needs a workspace origin to derive the viewer from', async () => {
    await expect(
      createPdfPrinter('https://example.com', () => ({ printFileToPdf }))(
        'a.html',
        { landscape: false },
      ),
    ).rejects.toThrow('the workspace has no viewer')
    expect(document.body.querySelector('iframe')).toBeNull()
  })
})

describe('readBigBuffer', () => {
  beforeAll(() => {
    ;(globalThis as unknown as { Mojo: object }).Mojo = { RESULT_OK: 0 }
  })

  it('reads inline bytes', () => {
    expect(readBigBuffer({ bytes: kPdf } as never)).toEqual(
      new Uint8Array(kPdf),
    )
  })

  it('copies shared memory out of the mapping', () => {
    const mapped = new Uint8Array(kPdf).buffer
    const mapBuffer = jest.fn(() => ({ result: 0, buffer: mapped }))
    const bytes = readBigBuffer({
      sharedMemory: { bufferHandle: { mapBuffer }, size: kPdf.length },
    } as never)
    expect(mapBuffer).toHaveBeenCalledWith(0, kPdf.length)
    expect(bytes).toEqual(new Uint8Array(kPdf))
    expect(bytes.buffer).not.toBe(mapped)
  })

  it('fails for shared memory that cannot be mapped', () => {
    const mapBuffer = () => ({ result: 3, buffer: new ArrayBuffer(0) })
    expect(() =>
      readBigBuffer({
        sharedMemory: { bufferHandle: { mapBuffer }, size: 4 },
      } as never),
    ).toThrow('could not map the PDF')
  })

  it('fails for an invalid buffer', () => {
    expect(() => readBigBuffer({ invalidBuffer: true } as never)).toThrow(
      'the browser returned no PDF',
    )
  })
})
