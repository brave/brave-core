// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
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
`

export const Button = styled(WalletButton)<{
  morePadding?: boolean
  isNFT: boolean
}>`
  --button-background-hover: #f5f6fc;
  @media (prefers-color-scheme: dark) {
    --button-background-hover: ${leo.color.container.background};
  }
  cursor: pointer;
  display: flex;
  outline: none;
  border: none;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: transparent;
  border-radius: ${(p) => (p.isNFT ? 8 : 12)}px;
  justify-content: center;
  padding: ${(p) => (p.morePadding ? 10 : 8)}px 12px;
  white-space: nowrap;
  :disabled {
    cursor: not-allowed;
  }
  &:hover {
    background-color: var(--button-background-hover);
  }
`

export const ButtonIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
  margin-left: 8px;
`

export const IconsWrapper = styled.div<{
  marginRight?: number
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: ${(p) => p.marginRight || 6}px;
`

export const ButtonText = styled(Text)<{
  isNFT: boolean
  isPlaceholder: boolean
}>`
  max-width: ${(p) => (p.isNFT ? '100%' : 'unset')};
  overflow: hidden;
  color: ${(p) =>
    p.isPlaceholder ? leo.color.text.tertiary : leo.color.text.primary};
  white-space: ${(p) => (p.isNFT ? 'pre-wrap' : 'nowrap')};
  word-break: break-all;
  font-weight: 500;
`
