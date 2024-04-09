// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

export const PasswordMatchRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  height: 20px;
`

export const PasswordValidationText = styled.p<{ isMatch: boolean }>`
  color: ${(p) =>
    p.isMatch
      ? leo.color.text.interactive
      : leo.color.systemfeedback.errorText};
  font: ${leo.font.small.regular};
  margin: 0;
  padding: 0;
`

export const PasswordValidationIcon = styled(Icon)<{ isMatch: boolean }>`
  --leo-icon-size: 14px;
  --leo-icon-color: ${(p) =>
    p.isMatch
      ? leo.color.text.interactive
      : leo.color.systemfeedback.errorIcon};

  display: inline-block;
  margin-right: 4px;
  vertical-align: middle;
`

export const TooltipWrapper = styled.div`
  display: flex;
  position: relative;
  flex-direction: row;
  justify-content: flex-end;
  width: calc(100% - 90px);
`
