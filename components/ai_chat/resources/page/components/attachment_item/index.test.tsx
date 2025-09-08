// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import {
  AttachmentItem,
  AttachmentPageItem,
  AttachmentSpinnerItem,
  AttachmentUploadItems,
  formatFileSize,
} from '.'
import * as Mojom from '../../../common/mojom'

// Mock URL.createObjectURL for tests that include image files
// This is needed because AttachmentUploadItems calls URL.createObjectURL to create blob URLs for images
Object.defineProperty(URL, 'createObjectURL', {
  writable: true,
  value: jest.fn(() => 'mock-object-url'),
})

describe('attachment item', () => {
  it('renders title, subtitle and thumbnail', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        icon={<img src='https://example.com/image.jpg' />}
      />,
    )
    expect(
      screen.getByText('Title', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(screen.getByText('Subtitle')).toBeInTheDocument()
    expect(
      container.querySelector('img[src="https://example.com/image.jpg"]'),
    ).toBeTruthy()

    expect(document.querySelector('leo-button')).not.toBeInTheDocument()
  })

  it('does not render subtitle if it is not provided', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle=''
        icon={<img src='https://example.com/image.jpg' />}
      />,
    )
    expect(
      container.querySelector('[data-key="subtitle"]'),
    ).not.toBeInTheDocument()
  })

  it('renders a remove button if necessary', async () => {
    const clickHandler = jest.fn()
    render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        icon={<img src='https://example.com/image.jpg' />}
        remove={clickHandler}
      />,
    )

    const button: any = document.querySelector('leo-button')!
    expect(button).toBeInTheDocument()
    expect(button).toBeVisible()

    // Button is a leo-button, we we trigger the click from the shadowRoot
    button?.shadowRoot?.querySelector('button').click()

    await waitFor(() => {
      expect(clickHandler).toHaveBeenCalled()
    })
  })
})

describe('attachment spinner item', () => {
  it('renders a loading spinner and title', async () => {
    const { container } = render(<AttachmentSpinnerItem title='Loading...' />)
    const spinner = container.querySelector('leo-progressring')
    expect(
      screen.getByText('Loading...', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(spinner).toBeInTheDocument()
    expect(spinner).toBeVisible()
  })
})

describe('attachment page item', () => {
  it('renders a page item', () => {
    const { container } = render(
      <AttachmentPageItem
        title='Title'
        url='https://example.com'
      />,
    )
    expect(
      screen.getByText('Title', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(
      screen.getByText('https://example.com', {
        selector: 'leo-tooltip [slot="content"]',
      }),
    ).toBeInTheDocument()
    expect(
      screen.getByText('example.com', {
        selector: '.subtitleText div:first-child',
      }),
    ).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="example.com"]'),
    ).toBeInTheDocument()
  })

  it('scheme is not hidden in tooltip', () => {
    render(
      <AttachmentPageItem
        title='Title'
        url='https://example.com'
      />,
    )
    expect(
      screen.getByText('https://example.com', {
        selector: 'leo-tooltip [slot="content"]',
      }),
    ).toBeInTheDocument()
  })

  it('does not hide non-http(s) urls', () => {
    render(
      <AttachmentPageItem
        title='Title'
        url='brave://newtab'
      />,
    )
    expect(
      screen.getByText('brave://newtab', {
        selector: 'leo-tooltip div:first-child',
      }),
    ).toBeInTheDocument()

    render(
      <AttachmentPageItem
        title='Title'
        url='file:///path/to/file.txt'
      />,
    )
    expect(
      screen.getByText('file:///path/to/file.txt', {
        selector: 'leo-tooltip div:first-child',
      }),
    ).toBeInTheDocument()

    render(
      <AttachmentPageItem
        title='Title'
        url='ftp:///path/to/file.txt'
      />,
    )
    expect(
      screen.getByText('ftp:///path/to/file.txt', {
        selector: 'leo-tooltip div:first-child',
      }),
    ).toBeInTheDocument()
  })
})

describe('formatFileSize', () => {
  it('should format bytes correctly', () => {
    expect(formatFileSize(512)).toBe('512.00 B')
    expect(formatFileSize(1024)).toBe('1.00 KB')
    expect(formatFileSize(1025)).toBe('1.00 KB')
    expect(formatFileSize(1025 * 1024)).toBe('1.00 MB')
    expect(formatFileSize(1536)).toBe('1.50 KB')
    expect(formatFileSize(1024 * 1024)).toBe('1.00 MB')
    expect(formatFileSize(1025 * 1024 * 1024)).toBe('1.00 GB')
    expect(formatFileSize(1024 * 1024 * 1024)).toBe('1.00 GB')
  })

  it('should handle large file sizes', () => {
    expect(formatFileSize(4.5 * 1024 * 1024)).toBe('4.50 MB')
    expect(formatFileSize(2.5 * 1024 * 1024 * 1024)).toBe('2.50 GB')
  })

  it('should handle edge cases', () => {
    expect(formatFileSize(0)).toBe('0.00 B')
    expect(formatFileSize(1023)).toBe('1023.00 B')
    expect(formatFileSize(1024 * 1024 * 1024 * 1024)).toBe('1024.00 GB') // Max supported unit
  })
})

describe('AttachmentUploadItems', () => {
  const createMockFile = (
    filename: string,
    type: Mojom.UploadedFileType,
  ): Mojom.UploadedFile => ({
    filename,
    type,
    data: new ArrayBuffer(100),
    filesize: BigInt(100),
  })

  it('renders regular image files with original filename', () => {
    const uploadedFiles = [
      createMockFile('photo.jpg', Mojom.UploadedFileType.kImage),
    ]

    render(<AttachmentUploadItems uploadedFiles={uploadedFiles} />)

    expect(screen.getByText('photo.jpg')).toBeInTheDocument()
  })

  it('renders screenshot with localized title', () => {
    const uploadedFiles = [
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
    ]

    render(<AttachmentUploadItems uploadedFiles={uploadedFiles} />)

    expect(
      screen.getByText('CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE'),
    ).toBeInTheDocument()
  })

  it('renders only the first screenshot when multiple screenshots exist', () => {
    const uploadedFiles = [
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}1.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}2.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
    ]

    const { container } = render(
      <AttachmentUploadItems uploadedFiles={uploadedFiles} />,
    )

    // Should only find one "CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE" title
    const screenshotTitles = screen.getAllByText(
      'CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE',
    )
    expect(screenshotTitles).toHaveLength(1)

    // Should only have one image rendered
    const images = container.querySelectorAll('img')
    expect(images).toHaveLength(1)
  })

  it('renders mixed file types correctly', () => {
    const uploadedFiles = [
      createMockFile('photo.jpg', Mojom.UploadedFileType.kImage),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}1.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile('document.pdf', Mojom.UploadedFileType.kPdf),
    ]

    render(<AttachmentUploadItems uploadedFiles={uploadedFiles} />)

    expect(screen.getByText('photo.jpg')).toBeInTheDocument()
    expect(
      screen.getByText('CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE'),
    ).toBeInTheDocument()
    expect(screen.getByText('document.pdf')).toBeInTheDocument()

    // Should only have one screenshot title despite multiple screenshot files
    const screenshotTitles = screen.getAllByText(
      'CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE',
    )
    expect(screenshotTitles).toHaveLength(1)
  })

  it('removes all full page screenshots when screenshot thumbnail remove is clicked', () => {
    const mockRemove = jest.fn()
    const uploadedFiles = [
      createMockFile('image.jpg', Mojom.UploadedFileType.kImage),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}1.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}2.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile('document.pdf', Mojom.UploadedFileType.kPdf),
    ]

    render(
      <AttachmentUploadItems
        uploadedFiles={uploadedFiles}
        remove={mockRemove}
      />,
    )

    // Should render image, one screenshot thumbnail, and PDF
    expect(screen.getByText('image.jpg')).toBeInTheDocument()
    expect(
      screen.getByText('CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE'),
    ).toBeInTheDocument()
    expect(screen.getByText('document.pdf')).toBeInTheDocument()

    // Find the remove button for the screenshot thumbnail (should be the second button)
    const removeButtons = document.querySelectorAll('leo-button')
    expect(removeButtons).toHaveLength(3) // image, screenshot, pdf

    // Click the screenshot remove button (middle one)
    removeButtons[1]?.shadowRoot?.querySelector('button')?.click()

    // Should call remove with the actual index of the first full page screenshot (index 1)
    expect(mockRemove).toHaveBeenCalledTimes(1)
    expect(mockRemove).toHaveBeenCalledWith(1)
  })

  it('shows remove button for all file types when remove callback is provided', () => {
    const mockRemove = jest.fn()
    const uploadedFiles = [
      createMockFile('image.jpg', Mojom.UploadedFileType.kImage),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile('document.pdf', Mojom.UploadedFileType.kPdf),
    ]

    render(
      <AttachmentUploadItems
        uploadedFiles={uploadedFiles}
        remove={mockRemove}
      />,
    )

    // Should render all items
    expect(screen.getByText('image.jpg')).toBeInTheDocument()
    expect(
      screen.getByText('CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE'),
    ).toBeInTheDocument()
    expect(screen.getByText('document.pdf')).toBeInTheDocument()

    // Should have remove buttons for all file types
    const removeButtons = document.querySelectorAll('leo-button')
    expect(removeButtons).toHaveLength(3) // image, screenshot, and PDF all have remove buttons
  })

  it('treats non-fullscreenshot screenshots as regular images', () => {
    const uploadedFiles = [
      createMockFile(
        'regular_screenshot.png',
        Mojom.UploadedFileType.kScreenshot,
      ),
      createMockFile(
        `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
        Mojom.UploadedFileType.kScreenshot,
      ),
    ]

    render(<AttachmentUploadItems uploadedFiles={uploadedFiles} />)

    // Regular screenshot should show with its original filename
    expect(screen.getByText('regular_screenshot.png')).toBeInTheDocument()
    // Full page screenshot should show with localized title
    expect(
      screen.getByText('CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE'),
    ).toBeInTheDocument()
  })
})
