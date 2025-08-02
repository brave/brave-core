// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { AttachmentItem, AttachmentPageItem, AttachmentSpinnerItem, formatFileSize } from '.'

describe('attachment item', () => {
  it('renders title, subtitle and thumbnail', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        icon={<img src='https://example.com/image.jpg' />}
      />
    )
    expect(screen.getByText('Title', { selector: '.title' })).toBeInTheDocument()
    expect(screen.getByText('Subtitle')).toBeInTheDocument()
    expect(
      container.querySelector('img[src="https://example.com/image.jpg"]')
    ).toBeTruthy()

    expect(document.querySelector('leo-button')).not.toBeInTheDocument()
  })

  it('does not render subtitle if it is not provided', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle=''
        icon={<img src='https://example.com/image.jpg' />}
      />
    )
    expect(
      container.querySelector('[data-key="subtitle"]')
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
      />
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
    expect(screen.getByText('Loading...', { selector: '.title' })).toBeInTheDocument()
    expect(spinner).toBeInTheDocument()
    expect(spinner).toBeVisible()
  })
})

describe('attachment page item', () => {
  it('renders a page item', () => {
    const { container } = render(<AttachmentPageItem title='Title' url='https://example.com' />)
    expect(screen.getByText('Title', { selector: '.title' })).toBeInTheDocument()
    expect(screen.getByText('https://example.com', { selector: 'leo-tooltip [slot="content"]' })).toBeInTheDocument()
    expect(screen.getByText('example.com', { selector: '.subtitleText div:first-child' })).toBeInTheDocument()
    expect(container.querySelector('img[src*="example.com"]')).toBeInTheDocument()
  })

  it('scheme is not hidden in tooltip', () => {
    render(<AttachmentPageItem title='Title' url='https://example.com' />)
    expect(screen.getByText('https://example.com', { selector: 'leo-tooltip [slot="content"]' })).toBeInTheDocument()
  })

  it('does not hide non-http(s) urls', () => {
    render(<AttachmentPageItem title='Title' url='brave://newtab' />)
    expect(screen.getByText('brave://newtab', { selector: 'leo-tooltip div:first-child' })).toBeInTheDocument()

    render(<AttachmentPageItem title='Title' url='file:///path/to/file.txt' />)
    expect(screen.getByText('file:///path/to/file.txt', { selector: 'leo-tooltip div:first-child' })).toBeInTheDocument()

    render(<AttachmentPageItem title='Title' url='ftp:///path/to/file.txt' />)
    expect(screen.getByText('ftp:///path/to/file.txt', { selector: 'leo-tooltip div:first-child' })).toBeInTheDocument()
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
