// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import LeoIcon from '@brave/leo/react/icon'

// Shared Styles
import { Text } from '../../shared/style'

export const LoginWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  background-color: ${leo.color.systemfeedback.infoBackground};
  border-radius: 8px;
  padding: 8px 16px;
  width: 100%;
`

export const InfoText = styled(Text)`
  line-height: 24px;
  color: ${leo.color.text.tertiary};
  padding: 6px 0px;
`

export const InfoIcon = styled(LeoIcon).attrs({
  name: 'warning-circle-filled'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.infoIcon};
  margin-right: 16px;
`

export const ButtonWrapper = styled.div`
  min-width: 42px;
`
