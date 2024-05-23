// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  layoutPanelWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Column, Text, Row } from '../../../../components/shared/style'

export const Wrapper = styled(Column)`
  padding: 24px 24px 0px 24px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px 16px 0px 16px;
  }
`

export const BalanceText = styled(Text)`
  line-height: 22px;
  margin-right: 4px;
`

export const FromText = styled(BalanceText)`
  color: ${leo.color.text.tertiary};
`

export const NetworkText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.tertiary};
`

export const FiatText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.interactive};
`

export const AccountNameAndPresetsRow = styled(Row)`
  min-height: 26px;
`

export const SelectTokenAndInputRow = styled(Row)`
  min-height: 60px;
`

export const NetworkAndFiatRow = styled(Row)`
  min-height: 22px;
`

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.interactive};
  margin-right: 8px;
`

export const AccountNameAndBalanceRow = styled(Row)`
  flex-wrap: wrap;
`
