// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  Row,
  WalletButton,
  Text
} from '../../../../components/shared/style'

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
  z-index: 3;
`

export const ButtonWrapper = styled(Row)`
  --background-hover: ${leo.color.container.background};
  --shadow-hover: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    --background-hover: transparent;
    --shadow-hover: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  flex-direction: row;
  background-color: transparent;
  border-radius: 8px;
  justify-content: space-between;
  white-space: nowrap;
  margin-bottom: 8px;
  &:hover {
    background-color: var(--background-hover);
    box-shadow: var(--shadow-hover);
  }
`

export const Button = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  outline: none;
  border: none;
  background-color: transparent;
  justify-content: space-between;
  align-items: center;
  padding: 8px;
  white-space: nowrap;
  width: 100%;
  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: 16px;
`

export const IconAndName = styled(Row)`
  width: 80%;
  white-space: pre-line;
`

export const NameAndBalanceText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.primary};
`

export const NetworkAndFiatText = styled(Text)`
  line-height: 20px;
  color: ${leo.color.text.secondary};
`

export const DisabledLabel = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  background-color: ${leo.color.green[20]};
  color: ${leo.color.green[50]};
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  font-family: 'Poppins';
  padding: 4px 6px;
  border-radius: 4px;
`
