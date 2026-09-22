// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Assets
import CryptoCardGraphic from './crypto_card_background.png'

// Shared Styles
import { CardBackground } from '../cards.style'
import { Text } from '$wallet/components/shared/style'

export const CryptoCardBackground = styled(CardBackground)`
  background-image: url(${CryptoCardGraphic});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: center;
`

export const CryptoCardIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.icon.default};
`

export const CryptoTitle = styled(Text)`
  background: ${leo.gradient.iconsActive};
  background-clip: text;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
`
