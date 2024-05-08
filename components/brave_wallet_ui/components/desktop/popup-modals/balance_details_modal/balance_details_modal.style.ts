// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Column, Text } from '../../../shared/style'

export const BalancesColumn = styled(Column)`
  background-color: ${leo.color.page.background};
  border-radius: 16px;
  padding: 24px;
  gap: 16px;
`

export const SectionText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.secondary};
`

export const BalanceText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.primary};
`

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
`

export const TooltipContent = styled.div`
  max-width: 280px;
`
