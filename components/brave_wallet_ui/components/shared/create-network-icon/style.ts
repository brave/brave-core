// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { AssetIconFactory, AssetIconProps } from '../style'

export const IconWrapper = styled.div<{
  marginRight: number
  isTestnet: boolean
  size?: string
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: ${p => p.size ? p.size : '15px'};
  height: ${p => p.size ? p.size : '15px'};
  margin-right: ${(p) => `${p.marginRight}px`};
  filter: ${(p) => p.isTestnet ? 'grayscale(100%)' : 'none'};
`

export const Placeholder = styled.div<{ orb: string }>`
  width: 10px;
  height: 10px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

type IconSize = 'huge' | 'big' | 'small' | 'tiny' | 'extra-small'

interface IconProps extends AssetIconProps {
  size?: IconSize
}

function getNetworkIconWidthFromSize(size?: IconSize): string {
  switch (size) {
    case 'huge':
      return '32px'
    case 'big':
      return '24px'
    case 'small':
      return '15px'
    case 'tiny':
      return '12px'
    case 'extra-small':
      return '8px'
    default:
      return '15px' // small
  }
}

export const NetworkIcon = AssetIconFactory<IconProps>((props) => ({
  width: getNetworkIconWidthFromSize(props.size),
  height: 'auto'
}))
