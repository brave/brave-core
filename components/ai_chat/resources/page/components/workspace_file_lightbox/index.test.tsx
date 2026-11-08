// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, fireEvent, waitFor, act } from '@testing-library/react'
import { requestWorkspaceZip } from '../../../leo_workspace/download'
import WorkspaceFileLightbox, { hyphenateTitle, WorkspaceFileInfo } from '.'

jest.mock('@brave/leo/react/dialog', () => {
  return function MockDialog(
    props: React.PropsWithChildren<{ isOpen?: boolean }>,
  ) {
    return props.isOpen ? <div>{props.children}</div> : null
  }
})

jest.mock('../../../leo_workspace/download', () => ({
  requestWorkspaceZip: jest.fn(),
}))

Object.defineProperty(URL, 'createObjectURL', {
  writable: true,
  value: jest.fn(() => 'mock-object-url'),
})

Object.defineProperty(URL, 'revokeObjectURL', {
  writable: true,
  value: jest.fn(),
})

const kFile: WorkspaceFileInfo = {
  workspaceUrl: 'workspace://a-uuid',
  filePath: 'src/main.ts',
}

const kDownloadSelector =
  'leo-button[title="CHAT_UI_WORKSPACE_FILE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL"]'

function renderLightbox(
  file: WorkspaceFileInfo | null = kFile,
  conversationTitle?: string,
) {
  return render(
    <WorkspaceFileLightbox
      file={file}
      conversationTitle={conversationTitle}
      onClose={jest.fn()}
      onOpen={jest.fn()}
    />,
  )
}

function downloadButton() {
  return document.querySelector(kDownloadSelector) as HTMLElement & {
    isDisabled?: boolean
    isLoading?: boolean
  }
}

describe('WorkspaceFileLightbox download', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('frames the workspace viewer for the file', () => {
    renderLightbox()
    expect(document.querySelector('iframe')).toHaveAttribute(
      'src',
      'chrome-untrusted://a-uuid.leo-workspace/#file=src%2Fmain.ts',
    )
  })

  it('is disabled until the workspace frame has loaded', () => {
    renderLightbox()
    expect(downloadButton().isDisabled).toBe(true)
    fireEvent.load(document.querySelector('iframe')!)
    expect(downloadButton().isDisabled).toBe(false)
  })

  /**
   * Clicks download with the workspace answering with |zip|, and returns the
   * name the browser was asked to save it under.
   */
  async function download(zip: File, conversationTitle?: string) {
    ;(requestWorkspaceZip as jest.Mock).mockResolvedValue(zip)

    const click = jest.fn()
    let link: HTMLAnchorElement | undefined
    const createElement = document.createElement.bind(document)
    const spy = jest
      .spyOn(document, 'createElement')
      .mockImplementation((tagName: string) => {
        const el = createElement(tagName)
        if (tagName === 'a') {
          link = el as HTMLAnchorElement
          Object.defineProperty(el, 'click', { value: click })
        }
        return el
      })

    renderLightbox(kFile, conversationTitle)
    const iframe = document.querySelector('iframe')!
    fireEvent.load(iframe)

    await act(async () => {
      downloadButton().shadowRoot?.querySelector('button')?.click()
    })

    expect(requestWorkspaceZip).toHaveBeenCalledWith(
      iframe.contentWindow,
      'chrome-untrusted://a-uuid.leo-workspace',
      expect.any(AbortSignal),
    )
    await waitFor(() => expect(click).toHaveBeenCalled())
    expect(URL.createObjectURL).toHaveBeenCalledWith(zip)
    expect(downloadButton().isLoading).toBe(false)
    spy.mockRestore()
    return link!.download
  }

  it('names the zip after the conversation title', async () => {
    const zip = new File(['zip'], 'my-folder.zip')
    expect(await download(zip, 'Build me a Landing Page!')).toBe(
      'build-me-a-landing-page.zip',
    )
  })

  it.each([[undefined], [''], ['?!']])(
    'falls back to the folder name for title %p',
    async (title) => {
      const zip = new File(['zip'], 'my-folder.zip')
      expect(await download(zip, title)).toBe('my-folder.zip')
    },
  )

  it('shows progress while the zip is being built', async () => {
    const { promise, resolve } = Promise.withResolvers<File>()
    ;(requestWorkspaceZip as jest.Mock).mockReturnValue(promise)

    renderLightbox()
    fireEvent.load(document.querySelector('iframe')!)
    await act(async () => {
      downloadButton().shadowRoot?.querySelector('button')?.click()
    })
    expect(downloadButton().isLoading).toBe(true)

    await act(async () => {
      resolve(new File([], 'workspace.zip'))
    })
    expect(downloadButton().isLoading).toBe(false)
  })

  it('abandons the download when the lightbox closes', async () => {
    ;(requestWorkspaceZip as jest.Mock).mockReturnValue(new Promise(() => {}))

    const { rerender } = renderLightbox()
    fireEvent.load(document.querySelector('iframe')!)
    await act(async () => {
      downloadButton().shadowRoot?.querySelector('button')?.click()
    })
    const signal = (requestWorkspaceZip as jest.Mock).mock
      .calls[0][2] as AbortSignal
    expect(signal.aborted).toBe(false)

    rerender(
      <WorkspaceFileLightbox
        file={null}
        onClose={jest.fn()}
        onOpen={jest.fn()}
      />,
    )
    expect(signal.aborted).toBe(true)
  })
})

describe('hyphenateTitle', () => {
  it.each([
    ['Build me a Landing Page!', 'build-me-a-landing-page'],
    ['  leading and trailing  ', 'leading-and-trailing'],
    ['a/b\\c:d*e?f"g<h>i|j', 'a-b-c-d-e-f-g-h-i-j'],
    ['Café über 2026', 'café-über-2026'],
    ['東京 の 旅行', '東京-の-旅行'],
    ['..', ''],
    ['', ''],
  ])('hyphenates %p as %p', (title, expected) => {
    expect(hyphenateTitle(title)).toBe(expected)
  })

  it('caps the length without leaving a trailing hyphen', () => {
    const result = hyphenateTitle('word '.repeat(50))
    expect(Array.from(result).length).toBeLessThanOrEqual(100)
    expect(result).not.toMatch(/^-|-$/)
  })
})
