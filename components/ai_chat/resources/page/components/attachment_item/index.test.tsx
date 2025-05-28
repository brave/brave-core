// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { AttachmentItem, AttachmentSpinnerItem } from '.'

describe('attachment item', () => {
  it('renders title, subtitle and thumbnail', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        icon={<img src='https://example.com/image.jpg' />}
      />,
    )
    expect(screen.getByText('Title')).toBeInTheDocument()
    expect(screen.getByText('Subtitle')).toBeInTheDocument()
    expect(
      container.querySelector('img[src="https://example.com/image.jpg"]'),
    ).toBeTruthy()

    expect(document.querySelector('leo-button')).not.toBeInTheDocument()
  })

  it('has a tooltip around the title', () => {
    const { container } = render(
      <AttachmentItem
        title='Title'
        subtitle='Subtitle'
        icon={<img src='https://example.com/image.jpg' />}
      />,
    )

    const tooltip = container.querySelector('leo-tooltip')
    expect(tooltip).toBeTruthy()
    expect(tooltip?.getAttribute('text')).toBe('Title')
    expect(tooltip?.textContent).toBe('Title')
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
    expect(screen.getByText('Loading...')).toBeInTheDocument()
    expect(spinner).toBeInTheDocument()
    expect(spinner).toBeVisible()
  })
})
