// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../../../components/shared/style'
import {
  layoutSmallWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const Button = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: none;
  outline: none;
  background-color: ${leo.color.container.background};
  box-shadow: 0px 1px 4px 0px rgba(0, 0, 0, 0.07);
  position: absolute;
  z-index: 1;
  cursor: pointer;
`

export const FlipButton = styled(Button)`
  padding: 12px;
  border-radius: 12px;
  left: 24px;
  :disabled {
    cursor: not-allowed;
  }
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: 16px;
  }
`

export const ProvidersButton = styled(FlipButton)`
  right: 24px;
  left: unset;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: unset;
    right: 16px;
  }
`

export const ProviderIcon = styled.img`
  height: 20px;
  width: auto;
`

export const FlipIcon = styled(Icon).attrs({
  name: 'swap-vertical'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
