// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, fireEvent, act } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import ImageLightbox from '.'

jest.mock('@brave/leo/react/dialog', () => {
  return function MockDialog(
    props: React.PropsWithChildren<{
      isOpen?: boolean
      className?: string
      style?: string
    }>,
  ) {
    if (!props.isOpen) {
      return null
    }
    return (
      <div
        className={props.className}
        data-dialog-style={props.style}
      >
        {props.children}
      </div>
    )
  }
})

Object.defineProperty(URL, 'createObjectURL', {
  writable: true,
  value: jest.fn(() => 'mock-object-url'),
})

Object.defineProperty(URL, 'revokeObjectURL', {
  writable: true,
  value: jest.fn(),
})

const mockFile: Mojom.UploadedFile = {
  filename: 'photo.jpg',
  type: Mojom.UploadedFileType.kImage,
  data: new ArrayBuffer(100) as unknown as number[],
  filesize: 20480,
  extractedText: undefined,
}

describe('ImageLightbox', () => {
  beforeEach(() => {
    jest.clearAllMocks()
    ;(URL.createObjectURL as jest.Mock).mockReturnValue('mock-object-url')
  })

  it('does not render content when file is null', () => {
    const { container } = render(
      <ImageLightbox
        file={null}
        onClose={jest.fn()}
      />,
    )
    expect(container.querySelector('img')).not.toBeInTheDocument()
  })

  it('renders image preview with filename and size', () => {
    render(
      <ImageLightbox
        file={mockFile}
        onClose={jest.fn()}
      />,
    )

    expect(screen.getByText('photo.jpg')).toBeInTheDocument()
    expect(screen.getByText('20.00 KB')).toBeInTheDocument()
    expect(screen.getByRole('img', { name: 'photo.jpg' })).toHaveAttribute(
      'src',
      'mock-object-url',
    )
  })

  it('sizes the dialog width from the image aspect ratio', () => {
    const { container } = render(
      <ImageLightbox
        file={mockFile}
        onClose={jest.fn()}
      />,
    )

    const image = screen.getByRole('img', { name: 'photo.jpg' })
    Object.defineProperty(image, 'naturalWidth', { value: 800 })
    Object.defineProperty(image, 'naturalHeight', { value: 400 })
    act(() => {
      fireEvent.load(image)
    })

    // 800x400 fits in 720x720 max → dialog width 720 (height follows via
    // height: auto on the image, preserving aspect ratio).
    expect(container.querySelector('[data-dialog-style]')).toHaveAttribute(
      'data-dialog-style',
      '--leo-dialog-width: 720px',
    )
  })

  it('copies the image to the clipboard', async () => {
    jest.useFakeTimers()
    const write = jest.fn().mockResolvedValue(undefined)
    Object.assign(navigator, {
      clipboard: { write },
    })
    ;(globalThis as any).ClipboardItem = class ClipboardItem {
      constructor(public items: Record<string, Blob>) {}
    }

    render(
      <ImageLightbox
        file={mockFile}
        onClose={jest.fn()}
      />,
    )

    const copyButton = document.querySelector(
      'leo-button[title="CHAT_UI_IMAGE_LIGHTBOX_COPY_BUTTON_LABEL"]',
    ) as HTMLElement
    expect(copyButton).toBeTruthy()
    expect(copyButton.querySelector('leo-icon[name="copy"]')).toBeTruthy()

    await act(async () => {
      copyButton.shadowRoot?.querySelector('button')?.click()
    })

    await waitFor(() => {
      expect(write).toHaveBeenCalled()
    })
    expect(
      copyButton.querySelector('leo-icon[name="check-normal"]'),
    ).toBeTruthy()
    expect(copyButton.className).toContain('copyButtonSuccess')

    act(() => {
      jest.advanceTimersByTime(2000)
    })

    expect(copyButton.querySelector('leo-icon[name="copy"]')).toBeTruthy()
    expect(copyButton.className).not.toContain('copyButtonSuccess')
    jest.useRealTimers()
  })

  it('downloads the image when download is clicked', () => {
    const click = jest.fn()
    const createElement = document.createElement.bind(document)
    jest.spyOn(document, 'createElement').mockImplementation((tagName) => {
      const el = createElement(tagName)
      if (tagName === 'a') {
        Object.defineProperty(el, 'click', { value: click })
      }
      return el
    })

    render(
      <ImageLightbox
        file={mockFile}
        onClose={jest.fn()}
      />,
    )

    const downloadButton = document.querySelector(
      'leo-button[title="CHAT_UI_IMAGE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL"]',
    ) as HTMLElement
    expect(downloadButton).toBeTruthy()
    downloadButton.shadowRoot?.querySelector('button')?.click()

    expect(click).toHaveBeenCalled()
    ;(document.createElement as jest.Mock).mockRestore()
  })
})
