// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'

// Mock Data
import {
  mockMeldCryptoQuotes,
  mockServiceProviders,
} from '../../../../../common/constants/mocks'

// Components
import { BuyQuote } from './buy_quote'

describe('buy quote option', () => {
  it('renders buy quote option and handles click', () => {
    const clickHandler = jest.fn()
    const { container } = render(
      <BuyQuote
        quote={mockMeldCryptoQuotes[0]}
        serviceProviders={mockServiceProviders}
        isCreatingWidget={false}
        isBestOption={true}
        isOpenOverride={true}
        onBuy={clickHandler}
      />,
    )

    // Test Text
    expect(screen.getByText('Transak')).toBeInTheDocument()
    expect(screen.getByText('braveWalletBestOption')).toBeInTheDocument()

    // Test Styles
    const styledWrapper = container.querySelector(
      '[data-key="buy-quote-wrapper"]',
    )
    expect(styledWrapper).toBeInTheDocument()
    expect(styledWrapper).toHaveStyle({
      boxSizing: 'border-box',
      overflow: 'hidden',
    })

    // Test Carat Icon
    const caratIcon = container.querySelector('leo-icon[name="carat-down"]')
    expect(caratIcon).toBeInTheDocument()
    expect(caratIcon).toBeVisible()
    expect(caratIcon).toHaveAttribute('name', 'carat-down')

    // Test Buy Button Click
    const buyButton: any = document.querySelector('leo-button')
    expect(buyButton).toBeInTheDocument()
    expect(buyButton).toBeVisible()

    // Button is a leo-button, we we trigger the click from the shadowRoot
    buyButton?.shadowRoot?.querySelector('button').click()
    expect(clickHandler).toHaveBeenCalledWith(mockMeldCryptoQuotes[0])
  })
})
