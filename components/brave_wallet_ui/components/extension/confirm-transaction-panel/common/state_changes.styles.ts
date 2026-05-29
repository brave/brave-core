// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color } from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text } from '../../../shared/style'

export const StateChangeText = styled(Text)`
  vertical-align: middle;
  text-align: left;
  display: inline-flex;
  flex-wrap: wrap;
  gap: 4px;

  strong {
    font-weight: 600;
    word-break: break-all;
    color: var(--primary);
  }
`

export const ArrowRightIcon = styled(Icon).attrs({
  name: 'arrow-right',
})`
  display: inline-block;
  --leo-icon-size: 14px;
  height: 14px;
  padding-top: 2px;
  align-self: center;
  color: ${color.icon.default};
`

export const UnverifiedTokenIndicator = styled.div`
  border-radius: 100%;
  background-color: ${color.systemfeedback.errorIcon};
  width: 10px;
  height: 10px;
`

export const TooltipContent = styled.div`
  max-width: 200px;
  word-break: break-all;
`
