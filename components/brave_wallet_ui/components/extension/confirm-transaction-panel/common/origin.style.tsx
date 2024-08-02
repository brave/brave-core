// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// shared styles
import { Row, Column } from '../../../shared/style'

export const OriginURLText = styled.span`
  word-break: break-word;
  text-align: left;
  margin-bottom: 4px;
  color: ${leo.color.neutral[60]};
  font-size: 12px;
  margin-bottom: 0px;
`

export const ContractOriginColumn = styled(Column)`
  text-align: left;
`

export const InlineContractRow = styled(Row)`
  display: inline-flex;
  justify-content: flex-start;
  align-items: center;
  flex-direction: row;
  text-align: left;
  vertical-align: center;
  font: ${leo.font.xSmall.regular};
  gap: 4px;
`

export const OriginIndicatorIconWrapper = styled.div`
  position: absolute;
  bottom: 4px;
  right: 0px;
`

export const OriginWarningIndicator = styled.div`
  width: 10px;
  height: 10px;
  border: 1.2px ${leo.color.container.background} solid;
  border-radius: 100%;
  background-color: ${leo.color.systemfeedback.errorIcon};
`
