// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

// Utils
import {
  getRewardsProviderBackground
} from '../../../utils/rewards_utils'

// Types
import {
  ExternalWalletProvider
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

export const AccountBox = styled.div<{
  orb?: string
  size?: 'big' | 'medium' | 'small' | 'tiny'
  marginRight?: number
  round?: boolean
}>`
  --box-big: 48px;
  --box-medium: 32px;
  --box-small: 24px;
  --box-tiny: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  min-width: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  height: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  border-radius: ${(p) =>
    p.round
      ? '50%'
      : p.size === 'big'
        ? '8px'
        : '4px'
  };
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: ${(p) =>
    p.marginRight !== undefined
      ? p.marginRight
      : 0
  }px;
`

export const AccountIcon = styled(Icon) <{
  size?: 'big' | 'medium' | 'small' | 'tiny'
}>`
  --icon-big: 24px;
  --icon-medium: 20px;
  --icon-small: 16px;
  --icon-tiny: 14px;
  --leo-icon-size: ${(p) =>
    p.size
      ? `var(--icon-${p.size})`
      : 'var(--icon-medium)'
  };
  color: ${leo.color.white};
`

export const ExternalAccountBox = styled(AccountBox) <{
  provider: ExternalWalletProvider | null
}>`
  background-color: ${(p) => p.provider
    ? getRewardsProviderBackground(p.provider)
    : 'none'
  };
  background-image: unset;
  background-size: unset;
`

export const ExternalAccountIcon = styled.img<{
  size?: 'big' | 'medium' | 'small' | 'tiny'
}>`
  --icon-big: 32px;
  --icon-medium: 24px;
  --icon-small: 18px;
  --icon-tiny: 14px;
  width: ${(p) =>
    p.size
      ? `var(--icon-${p.size})`
      : 'var(--icon-medium)'
  };
  height: ${(p) =>
    p.size
      ? `var(--icon-${p.size})`
      : 'var(--icon-medium)'
  };
`
