// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getLocale } from '../../../../common/locale'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  onClickDeposit: () => void
  onClickViewOnExplorer?: () => void
  onClickSell?: () => void
}

export const PortfolioAccountMenu = (props: Props) => {
  const { onClickSell, onClickViewOnExplorer, onClickDeposit } = props

  return (
    <ButtonMenu placement='bottom-end'>
      <Button
        fab
        slot='anchor-content'
        kind='plain-faint'
        size='large'
      >
        <Icon name='more-vertical' />
      </Button>
      {onClickSell && (
        <leo-menu-item onClick={onClickSell}>
          <Icon name='usd-circle' />
          {getLocale('braveWalletSell')}
        </leo-menu-item>
      )}
      {onClickViewOnExplorer && (
        <leo-menu-item onClick={onClickViewOnExplorer}>
          <Icon name='launch' />
          {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
        </leo-menu-item>
      )}
      <leo-menu-item onClick={onClickDeposit}>
        <Icon name='money-bag-coins' />
        {getLocale('braveWalletDepositCryptoButton')}
      </leo-menu-item>
    </ButtonMenu>
  )
}
