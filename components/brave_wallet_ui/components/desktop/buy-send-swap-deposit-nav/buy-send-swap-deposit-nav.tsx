// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import { BuySendSwapDepositOptions } from '../../../options/buy-send-swap-deposit-options'

// Components
import { BuySendSwapDepositButton } from './buy-send-swap-deposit-button/buy-send-swap-deposit-button'

// Styled Components
import { Wrapper } from './buy-send-swap-deposit-nav.style'

// Transactions is not a option for Desktop.
const buttonOptions = BuySendSwapDepositOptions.filter((option) => option.id !== 'transactions')

export const BuySendSwapDepositNav = () => {
  return (
    <Wrapper>
      {buttonOptions.map((option) =>
        <BuySendSwapDepositButton option={option} key={option.id} />
      )}
    </Wrapper>
  )
}

export default BuySendSwapDepositNav
