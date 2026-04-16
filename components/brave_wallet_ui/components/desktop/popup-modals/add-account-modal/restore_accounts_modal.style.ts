// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import { Column, Row, Text } from '../../../shared/style'

export const ContentColumn = styled(Column)`
  padding: 0px 24px 24px;
`

export const RestoreAccountsList = styled(Column)`
  max-height: 320px;
  overflow: auto;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 8px;
  padding: 8px;
`

export const AccountNameText = styled(Text)`
  color: ${leo.color.text.primary};
  font-size: 14px;
  font-weight: 600;
`

export const AccountMetaText = styled(Text)`
  color: ${leo.color.text.secondary};
  font-size: 12px;
`

export const FooterRow = styled(Row)`
  justify-content: flex-end;
  gap: 12px;
  margin: 16px 0px 0px 0px;
`
