// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { AttachmentItem } from '.'
import styles from './index.module.scss'

describe('attachment item', () => {
  it('renders title, subtitle and thumbail', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        thumbnailUrl='https://example.com/image.jpg'
      />
    )
    expect(screen.getByText('Title')).toBeInTheDocument()
    expect(screen.getByText('Subtitle')).toBeInTheDocument()
    expect(
      container.querySelector('img[src="https://example.com/image.jpg"]')
    ).toBeTruthy()

    expect(document.querySelector('leo-button')).not.toBeInTheDocument()
  })

  it('has a tooltip around the title', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        thumbnailUrl='https://example.com/image.jpg'
      />
    )

    const tooltip = container.querySelector('leo-tooltip')
    expect(tooltip).toBeTruthy()
    expect(tooltip?.getAttribute('text')).toBe('Title')
    expect(tooltip?.textContent).toBe('Title')
  })

  it('renders a remove button if necessary', async () => {
    const clickHandler = jest.fn()
    render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        thumbnailUrl='https://example.com/image.jpg'
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

  it('can add small image', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        thumbnailUrl='https://example.com/image.jpg'
        smallImage
      />
    )

    const img = container.querySelector('img')
    expect(img).toBeInTheDocument()
    expect(img).toHaveClass(styles.image)
    expect(img).toHaveClass(styles.smallImage)
  })
})
