// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

interface Props {
  assetSymbol: string
  onClickTokenDetails: () => void
  onClickViewOnExplorer: () => void
  onClickHideToken: () => void
  onClickEditToken?: () => void
}

export const AssetDetailsMenu = (props: Props) => {
  const {
    assetSymbol,
    onClickTokenDetails,
    onClickViewOnExplorer,
    onClickHideToken,
    onClickEditToken
  } = props

  return (
    <StyledWrapper yPosition={42}>
      <PopupButton onClick={onClickTokenDetails}>
        <ButtonIcon name='info-outline' />
        <PopupButtonText>
          {getLocale('braveWalletPortfolioTokenDetailsMenuLabel')}
        </PopupButtonText>
      </PopupButton>
      {onClickEditToken && (
        <PopupButton onClick={onClickEditToken}>
          <ButtonIcon name='edit-pencil' />
          <PopupButtonText>
            {getLocale('braveWalletAllowSpendEditButton')}
          </PopupButtonText>
        </PopupButton>
      )}
      <PopupButton onClick={onClickViewOnExplorer}>
        <ButtonIcon name='launch' />
        <PopupButtonText>
          {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
        </PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickHideToken}>
        <ButtonIcon name='trash' />
        <PopupButtonText>
          {getLocale('braveWalletPortfolioHideTokenMenuLabel').replace(
            '$1',
            assetSymbol
          )}
        </PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}
