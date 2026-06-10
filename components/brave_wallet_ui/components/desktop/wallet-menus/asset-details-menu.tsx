// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { getLocale } from '../../../../common/locale'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  children: React.ReactNode
  assetSymbol: string
  onClickTokenDetails: () => void
  onClickViewOnExplorer: () => void
  onClickHideToken: () => void
  onClickEditToken?: () => void
}

export const AssetDetailsMenu = (props: Props) => {
  const {
    children,
    assetSymbol,
    onClickTokenDetails,
    onClickViewOnExplorer,
    onClickHideToken,
    onClickEditToken,
  } = props

  return (
    <ButtonMenu placement='bottom-end'>
      {children}
      <leo-menu-item onClick={onClickTokenDetails}>
        <Icon name='info-outline' />
        {getLocale('braveWalletPortfolioTokenDetailsMenuLabel')}
      </leo-menu-item>
      {onClickEditToken && (
        <leo-menu-item onClick={onClickEditToken}>
          <Icon name='edit-pencil' />
          {getLocale('braveWalletAllowSpendEditButton')}
        </leo-menu-item>
      )}
      <leo-menu-item onClick={onClickViewOnExplorer}>
        <Icon name='launch' />
        {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
      </leo-menu-item>
      <leo-menu-item onClick={onClickHideToken}>
        <Icon name='trash' />
        {getLocale('braveWalletPortfolioHideTokenMenuLabel').replace(
          '$1',
          assetSymbol,
        )}
      </leo-menu-item>
    </ButtonMenu>
  )
}
