// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  HelpCenterIcon,
  ExplorerIcon,
  DeleteIcon
} from './asset-more-popup-styles'

interface Props {
  assetSymbol: string
  onClickTokenDetails: () => void
  onClickViewOnExplorer: () => void
  onClickHideToken: () => void
}

export const AssetMorePopup = (props: Props) => {
  const {
    assetSymbol,
    onClickTokenDetails,
    onClickViewOnExplorer,
    onClickHideToken
  } = props

  return (
    <StyledWrapper>
      <PopupButton onClick={onClickTokenDetails}>
        <HelpCenterIcon />
        <PopupButtonText>{getLocale('braveWalletPortfolioTokenDetailsMenuLabel')}</PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickViewOnExplorer}>
        <ExplorerIcon />
        <PopupButtonText>{getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}</PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickHideToken}>
        <DeleteIcon />
        <PopupButtonText>{getLocale('braveWalletPortfolioHideTokenMenuLabel').replace('$1', assetSymbol)}</PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}
