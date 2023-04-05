// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import {
  NavOptions,
  BuySendSwapDepositOptions
} from '../../../options/nav-options'

// Components
import { WalletNavButton } from './wallet-nav-button/wallet-nav-button'

// Styled Components
import { Wrapper, Section } from './wallet-nav.style'
import './nav-theme.css'

export interface Props {
  isSwap?: boolean
}

const BRAVE_SWAP_DATA_THEME_KEY = 'brave-swap-data-theme'

export const WalletNav = (props: Props) => {
  const { isSwap } = props

  // This needs to stay in the WalletNav to update the
  // data-theme after routing away from Swap.
  React.useEffect(() => {
    if (isSwap) {
      const userTheme = window.localStorage.getItem(BRAVE_SWAP_DATA_THEME_KEY)
      // Do nothing if user has not set a theme.
      if (userTheme === null) {
        return
      }
      // Update data-theme if user has selected a theme.
      document.documentElement.setAttribute('data-theme', userTheme)
      return
    }
    // Remove data-theme attribute if not on Swap screen.
    document.documentElement.removeAttribute('data-theme')
  }, [isSwap])

  return (
    <Wrapper>
      <Section showBorder={true}>
        {NavOptions.map((option) =>
          <WalletNavButton option={option} key={option.id} />
        )}
      </Section>
      <Section>
        {BuySendSwapDepositOptions.map((option) =>
          <WalletNavButton option={option} key={option.id} />
        )}
      </Section>
    </Wrapper>
  )
}

export default WalletNav
