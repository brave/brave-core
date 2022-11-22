// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import { NavOptions } from '../../../options/nav-options'

// Components
import { BuySendSwapDepositButton } from './buy-send-swap-deposit-button/buy-send-swap-deposit-button'

// Styled Components
import { Wrapper } from './buy-send-swap-deposit-nav.style'

export interface Props {
  isTab?: boolean
}

// Transactions is not an option for Desktop.
const buttonOptions = NavOptions.filter((option) => option.id !== 'transactions')

export const BuySendSwapDepositNav = (props: Props) => {
  const { isTab } = props

  const filteredButtonOptions = React.useMemo(() => {
    if (!isTab) {
      // Portfolio is not an option in the crypto view.
      return buttonOptions.filter((option) => option.id !== 'portfolio')
    }
    return buttonOptions
  }, [isTab])

  return (
    <Wrapper isTab={isTab}>
      {filteredButtonOptions.map((option) =>
        <BuySendSwapDepositButton isTab={isTab} option={option} key={option.id} />
      )}
    </Wrapper>
  )
}

export default BuySendSwapDepositNav
