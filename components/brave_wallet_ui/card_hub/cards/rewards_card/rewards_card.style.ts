// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Assets
import RewardsCardGraphic from './rewards_card_background.jpg'

// Shared Styles
import { CardBackground } from '../cards.style'
import { Text } from '$wallet/components/shared/style'

export const RewardsCardBackground = styled(CardBackground)`
  background-color: #ff1c00;
  background-image: url(${RewardsCardGraphic});
  background-repeat: no-repeat;
  background-size: 102% 102%;
  background-position: center;
`

export const IconWrapper = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 35px;
  height: 35px;
  border-radius: ${leo.radius.m};
  background-color: ${leo.color.white};
`

export const BraveIcon = styled(Icon)`
  --leo-icon-size: 26px;
`

export const VerifyTextWrapper = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  border-radius: ${leo.radius.full};
  padding: ${leo.spacing.s} ${leo.spacing.m};
  background-color: color-mix(
    in srgb,
    ${leo.color.white} 15%,
    transparent
  );
`

export const BalanceText = styled(Text)`
  text-transform: uppercase;
  opacity: 0.8;
`

export const DescriptionText = styled(Text)`
  opacity: 0.85;
`
