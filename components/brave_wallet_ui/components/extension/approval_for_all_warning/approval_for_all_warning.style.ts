// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoButton from '@brave/leo/react/button'

// Shared Styles
import { Text, Column } from '../../shared/style'

export const Header = styled(Text)`
  font: ${leo.font.heading.h2};
  letter-spacing: ${leo.typography.letterSpacing.headings};
`

export const Title = styled(Text)`
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
`

export const Description = styled(Text)`
  font: ${leo.font.default.regular};
  letter-spacing: ${leo.typography.letterSpacing.default};
  width: 100%;

  ul {
    margin: 8px 0 0;
    padding-left: 20px;
  }
`

export const Backdrop = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(33, 37, 41, 0.32);
  backdrop-filter: blur(4px);
  padding: 16px;
`

export const Warning = styled(Column)`
  --leo-icon-color: ${leo.color.systemfeedback.warningIcon};
  --leo-icon-size: 40px;
  background-color: ${leo.color.systemfeedback.warningBackground};
  border-radius: ${leo.radius.xl};
  box-shadow: ${leo.effect.elevation['01']};
  overflow: hidden;
`

export const Button = styled(LeoButton)`
  width: 100%;
`
