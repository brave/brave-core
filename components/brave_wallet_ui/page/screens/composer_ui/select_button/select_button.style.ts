// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory
} from '../../../../components/shared/style'
import { ButtonText } from '../shared_composer.style'

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

export const SelectButtonText = styled(ButtonText)<{
  isNFT: boolean
}>`
  max-width: ${(p) => (p.isNFT ? '100%' : 'unset')};
  white-space: ${(p) => (p.isNFT ? 'pre-wrap' : 'nowrap')};
`
