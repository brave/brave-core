// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Utils
import { getRewardsProviderBackground } from '../../../utils/rewards_utils'
import { ExternalWalletProvider } from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Shared Styles
import { AssetIconFactory, AssetIconProps } from '../style'

export const IconWrapper = styled.div<{
  marginRight: number
  isTestnet: boolean
  size?: string
  externalProvider?: ExternalWalletProvider | null
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: ${(p) => (p.size ? p.size : '15px')};
  height: ${(p) => (p.size ? p.size : '15px')};
  margin-right: ${(p) => `${p.marginRight}px`};
  filter: ${(p) => (p.isTestnet ? 'grayscale(100%)' : 'none')};
  background-color: ${(p) =>
    p.externalProvider
      ? getRewardsProviderBackground(p.externalProvider)
      : 'none'};
  border-radius: ${(p) => (p.externalProvider ? '100%' : 'none')};
  padding: ${(p) => (p.externalProvider ? '2px' : '0px')};
`

export type IconSize =
  | 'massive'
  | 'huge'
  | 'big'
  | 'medium'
  | 'small'
  | 'tiny'
  | 'extra-small'

interface IconProps extends AssetIconProps {
  size?: IconSize
  isExternalProvider?: boolean
}

function getNetworkIconWidthFromSize(size?: IconSize): string {
  switch (size) {
    case 'massive':
      return '64px'
    case 'huge':
      return '32px'
    case 'big':
      return '24px'
    case 'medium':
      return '18px'
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

function getExternalProviderIconSize(size?: IconSize): string {
  switch (size) {
    case 'huge':
      return '24px'
    case 'big':
      return '20px'
    case 'small':
      return '16px'
    case 'tiny':
      return '12px'
    case 'extra-small':
      return '8px'
    default:
      return '10px'
  }
}

export const NetworkIcon = AssetIconFactory<IconProps>((props) => ({
  width: props.isExternalProvider
    ? getExternalProviderIconSize(props.size)
    : getNetworkIconWidthFromSize(props.size),
  height: 'auto'
}))

export const Placeholder = styled.div<{
  orb: string
  size?: IconSize
}>`
  min-width: ${(p) => (p.size ? getNetworkIconWidthFromSize(p.size) : '15px')};
  min-height: ${(p) => (p.size ? getNetworkIconWidthFromSize(p.size) : '15px')};
  border-radius: 100%;
  background: ${(p) => p.orb};
  background-size: cover;
`
