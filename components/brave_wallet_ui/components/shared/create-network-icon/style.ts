// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { AssetIconFactory, AssetIconProps } from '../style'

export const IconWrapper = styled.div<{
  marginRight: number
  isTestnet: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 15px;
  height: 15px;
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

interface IconProps extends AssetIconProps {
  size?: 'big' | 'small'
}

export const NetworkIcon = AssetIconFactory<IconProps>(props => ({
  width: props.size === 'big' ? '24px' : '15px',
  height: 'auto'
}))
