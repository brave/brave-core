// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { WalletButton, Row } from '../../../../../components/shared/style'

export const Wrapper = styled(Row)<{ type?: 'error' | 'warning' | 'info' }>`
  background-color: ${(p) =>
    p.type === 'error'
      ? leo.color.systemfeedback.errorBackground
      : p.type === 'warning'
        ? leo.color.systemfeedback.warningBackground
        : leo.color.systemfeedback.infoBackground};
  border-radius: 8px;
`

export const LearnMoreLink = styled.a`
  font: ${leo.font.small.semibold};
  color: ${leo.color.button.background};
  margin: 0px;
  padding: 0px;
  text-decoration: none;
  cursor: pointer;
`

export const HowToSolveButton = styled(WalletButton)`
  font: ${leo.font.small.semibold};
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  color: ${leo.color.button.background};
  margin: 0px;
`

export const ErrorIcon = styled(Icon)<{ type: 'error' | 'warning' }>`
  --leo-icon-size: 20px;
  color: ${(p) =>
    p.type === 'error'
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};
  margin-right: 16px;
`
