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
  onClickDeposit: () => void
  onClickViewOnExplorer?: () => void
  onClickSell?: () => void
}

export const PortfolioAccountMenu = (props: Props) => {
  const { onClickSell, onClickViewOnExplorer, onClickDeposit } = props

  return (
    <StyledWrapper yPosition={42}>
      {onClickSell && (
        <PopupButton onClick={onClickSell}>
          <ButtonIcon name='usd-circle' />
          <PopupButtonText>{getLocale('braveWalletSell')}</PopupButtonText>
        </PopupButton>
      )}
      {onClickViewOnExplorer && (
        <PopupButton onClick={onClickViewOnExplorer}>
          <ButtonIcon name='launch' />
          <PopupButtonText>
            {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
          </PopupButtonText>
        </PopupButton>
      )}
      <PopupButton onClick={onClickDeposit}>
        <ButtonIcon name='qr-code' />
        <PopupButtonText>
          {getLocale('braveWalletDepositCryptoButton')}
        </PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}
