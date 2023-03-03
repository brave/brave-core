// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Icon, StyledButton, Text } from '../../shared.styles'

import {
  AssetIconProps,
  AssetIconFactory
} from '../../../../../components/shared/style'

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
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 100%;
  padding: 2px;
`

export const Button = styled(StyledButton) <{ morePadding?: boolean, isNFT: boolean }>`
  --button-background-hover: #f5f6fc;
  @media (prefers-color-scheme: dark) {
    --button-background-hover: ${(p) => p.theme.color.background01};
    }
  background-color: transparent;
  border-radius: ${(p) => p.isNFT ? 8 : 100}px;
  justify-content: center;
  padding: ${(p) => (p.morePadding ? 10 : 8)}px 12px;
  white-space: nowrap;
  height: ${(p) => p.isNFT ? '110px' : 'unset'};
  &:hover {
    background-color: var(--button-background-hover);
  }
`

export const ButtonIcon = styled(Icon)`
  background-color: ${(p) => p.theme.color.text01};
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

export const ButtonText = styled(Text) <{ isNFT: boolean }>`
  max-width: ${(p) => p.isNFT ? '250px' : 'unset'};
  overflow: hidden;
  white-space: ${(p) => p.isNFT ? 'pre-wrap' : 'nowrap'};
`
