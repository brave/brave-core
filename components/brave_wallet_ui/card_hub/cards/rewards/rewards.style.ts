// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { CardBackground } from '../cards.style'
import { Text } from '$wallet/components/shared/style'

export const RewardsBackground = styled(CardBackground)`
  background-color: rgb(26, 31, 56);
  background-image: radial-gradient(
    circle,
    color-mix(in srgb, ${leo.color.white} 10%, transparent) 1px,
    transparent 1px
  );
  background-size: 14px 14px;
`

export const BATIcon = styled(Icon)`
  --leo-icon-size: 20px;
`

export const EnableRewardsWrapper = styled.div`
  background-color: rgb(28, 28, 29);
  padding: ${leo.spacing.s} ${leo.spacing.m};
  border-radius: ${leo.radius.full};
  @media (prefers-color-scheme: dark) {
    background-color: rgb(228, 228, 229);
  }
`

export const EnableRewardsText = styled(Text)`
  color: ${leo.color.white};
  @media (prefers-color-scheme: dark) {
    color: ${leo.color.black};
  }
`
