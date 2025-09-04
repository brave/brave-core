// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Utils
import { getRewardsProviderBackground } from '../../../utils/rewards_utils'

// Types
import { ExternalWalletProvider } from '../../../../brave_rewards/resources/shared/lib/external_wallet'

export const AccountBox = styled.div<{
  orb?: string
  size?: 'huge' | 'big' | 'medium' | 'small' | 'tiny' | 'x-tiny'
  marginRight?: number
  round?: boolean
}>`
  --box-huge: 48px;
  --box-big: 40px;
  --box-medium: 32px;
  --box-small: 24px;
  --box-tiny: 20px;
  --box-x-tiny: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: ${(p) => (p.size ? `var(--box-${p.size})` : 'var(--box-medium)')};
  min-width: ${(p) => (p.size ? `var(--box-${p.size})` : 'var(--box-medium)')};
  height: ${(p) => (p.size ? `var(--box-${p.size})` : 'var(--box-medium)')};
  border-radius: ${(p) => (p.round ? '50%' : p.size === 'big' ? '8px' : '4px')};
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: ${(p) => (p.marginRight !== undefined ? p.marginRight : 0)}px;
`

export const ExternalAccountBox = styled(AccountBox)<{
  provider: ExternalWalletProvider | null
}>`
  background-color: ${(p) =>
    p.provider ? getRewardsProviderBackground(p.provider) : 'none'};
  background-image: unset;
  background-size: unset;
`

export const ExternalAccountIcon = styled.img<{
  size?: 'huge' | 'big' | 'medium' | 'small' | 'tiny' | 'x-tiny'
}>`
  --icon-huge: 32px;
  --icon-big: 28px;
  --icon-medium: 24px;
  --icon-small: 18px;
  --icon-tiny: 14px;
  --icon-x-tiny: 10px;
  width: ${(p) => (p.size ? `var(--icon-${p.size})` : 'var(--icon-medium)')};
  height: ${(p) => (p.size ? `var(--icon-${p.size})` : 'var(--icon-medium)')};
`
