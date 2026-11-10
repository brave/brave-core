// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  kDownloadWorkspaceRequest,
  kDownloadWorkspaceResponse,
  requestWorkspaceZip,
} from './download'

const kWorkspaceOrigin = 'chrome-untrusted://a-uuid.leo-workspace'

let target: { postMessage: jest.Mock }

beforeEach(() => {
  target = { postMessage: jest.fn() }
})

/** The requestId of the request sent to |target|. */
function sentRequestId(): unknown {
  expect(target.postMessage).toHaveBeenCalledTimes(1)
  const [data, origin] = target.postMessage.mock.calls[0]
  expect(origin).toBe(kWorkspaceOrigin)
  expect(data.type).toBe(kDownloadWorkspaceRequest)
  return data.requestId
}

/** Dispatches a reply as if it came from |source| at |origin|. */
function respond(
  data: unknown,
  origin = kWorkspaceOrigin,
  source: unknown = target,
) {
  window.dispatchEvent(
    new MessageEvent('message', { data, origin, source: source as never }),
  )
}

function request(signal?: AbortSignal) {
  return requestWorkspaceZip(
    target as unknown as Window,
    kWorkspaceOrigin,
    signal,
  )
}

describe('requestWorkspaceZip', () => {
  it('resolves with the archive from the matching response', async () => {
    const promise = request()
    const file = new File(['zip'], 'workspace.zip')
    respond({
      type: kDownloadWorkspaceResponse,
      requestId: sentRequestId(),
      file,
    })
    await expect(promise).resolves.toBe(file)
  })

  it('rejects with the error the workspace reports', async () => {
    const promise = request()
    respond({
      type: kDownloadWorkspaceResponse,
      requestId: sentRequestId(),
      error: 'boom',
    })
    await expect(promise).rejects.toThrow('boom')
  })

  it('ignores responses that are not for this request', async () => {
    const promise = request()
    const requestId = sentRequestId()
    const bad = { type: kDownloadWorkspaceResponse, requestId, error: 'bad' }
    respond(bad, 'chrome-untrusted://b-uuid.leo-workspace')
    respond(bad, kWorkspaceOrigin, window)
    respond({ ...bad, requestId: 'other' })
    respond({ ...bad, type: 'READ_FILE_RESULT' })
    respond(null)

    const file = new File(['zip'], 'workspace.zip')
    respond({ type: kDownloadWorkspaceResponse, requestId, file })
    await expect(promise).resolves.toBe(file)
  })

  it('rejects and stops listening when aborted', async () => {
    const controller = new AbortController()
    const promise = request(controller.signal)
    const requestId = sentRequestId()
    controller.abort(new Error('closed'))
    await expect(promise).rejects.toThrow('closed')
    // A late response must not throw or resolve anything.
    respond({
      type: kDownloadWorkspaceResponse,
      requestId,
      file: new File([], 'x'),
    })
  })

  it('does not send a request when already aborted', async () => {
    const controller = new AbortController()
    controller.abort(new Error('closed'))
    await expect(request(controller.signal)).rejects.toThrow('closed')
    expect(target.postMessage).not.toHaveBeenCalled()
  })
})
