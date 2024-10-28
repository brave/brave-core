/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { StyledCard } from '../widgetCard'

import { gradient, color, radius } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

export const PromoCard = styled(StyledCard)`
  background: ${gradient.panelBackground};
`
export const PromoButton = styled(Button)`
  background: ${color.material.divider};
  border-radius: ${radius.l};
`

export const CardHeaderIcon = styled(Icon)`
  --leo-icon-size: 24px;
  --leo-icon-color: ${gradient.iconsActive};
`
