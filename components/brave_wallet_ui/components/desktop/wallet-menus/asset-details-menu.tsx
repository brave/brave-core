// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'

// utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  PopupButton,
  MenuItemIcon,
  MenuButton,
  INTERACTIVE_ICON_COLOR,
  MenuItemRow
} from './wallet_menus.style'

interface Props {
  assetSymbol: string
  onClickTokenDetails: () => void
  onClickViewOnExplorer: () => void
  onClickHideToken: () => void
  onClickEditToken?: () => void
}

export const AssetDetailsMenu = ({
  assetSymbol,
  onClickTokenDetails,
  onClickViewOnExplorer,
  onClickHideToken,
  onClickEditToken
}: Props) => {
  // render
  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton
          kind='outline'
          padding='1px 4px'
        >
          <MenuItemIcon
            name='more-vertical'
            color={INTERACTIVE_ICON_COLOR}
          />
        </MenuButton>
      </div>

      <PopupButton onClick={onClickTokenDetails}>
        <MenuItemRow>
          <MenuItemIcon name='info-outline' />
          {getLocale('braveWalletPortfolioTokenDetailsMenuLabel')}
        </MenuItemRow>
      </PopupButton>
      {onClickEditToken && (
        <PopupButton onClick={onClickEditToken}>
          <MenuItemRow>
            <MenuItemIcon name='edit-pencil' />
            {getLocale('braveWalletAllowSpendEditButton')}
          </MenuItemRow>
        </PopupButton>
      )}
      <PopupButton onClick={onClickViewOnExplorer}>
        <MenuItemRow>
          <MenuItemIcon name='launch' />
          {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
        </MenuItemRow>
      </PopupButton>
      <PopupButton onClick={onClickHideToken}>
        <MenuItemRow>
          <MenuItemIcon name='trash' />
          {getLocale('braveWalletPortfolioHideTokenMenuLabel').replace(
            '$1',
            assetSymbol
          )}
        </MenuItemRow>
      </PopupButton>
    </ButtonMenu>
  )
}
