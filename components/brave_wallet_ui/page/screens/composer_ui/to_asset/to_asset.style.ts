// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text, Row, WalletButton } from '../../../../components/shared/style'

export const ReceiveAndQuoteText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
`

export const NetworkAndFiatText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.tertiary};
`

export const ReceiveAndQuoteRow = styled(Row)`
  min-height: 26px;
`

export const SelectAndInputRow = styled(Row)`
  min-height: 60px;
`

export const NetworkAndFiatRow = styled(Row)`
  min-height: 22px;
`

export const RefreshIcon = styled(Icon).attrs({
  name: 'refresh'
})`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
`

export const RefreshButton = styled(WalletButton)<{
  clicked: boolean
}>`
  cursor: pointer;
  border: none;
  background: none;
  padding: 0px;
  animation: ${(p) => (p.clicked ? 'spin 1s 1' : 'none')};
  @keyframes spin {
    0% {
      transform: rotate(0deg);
    }
    100% {
      transform: rotate(-360deg);
    }
  }
  &:disabled {
    cursor: not-allowed;
    opacity: 0.4;
  }
`
