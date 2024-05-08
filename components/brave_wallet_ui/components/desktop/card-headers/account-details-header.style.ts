// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text } from '../../shared/style'

export const AccountNameText = styled(Text)`
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const AddressText = styled(Text)`
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.primary};
  margin-right: 8px;
`

export const AccountsNetworkText = styled(Text)`
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
`

export const AccountBalanceText = styled(Text)`
  font-size: 22px;
  line-height: 32px;
  font-weight: 500;
  color: ${leo.color.text.primary};
`

export const CopyIcon = styled(Icon).attrs({
  name: 'copy'
})`
  cursor: pointer;
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};
`
