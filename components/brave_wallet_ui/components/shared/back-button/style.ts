// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'
import { WalletButton } from '../style'

// Will use brave-ui button comp in the future!
// Currently is missing "tiny" variant
export const StyledWrapper = styled(WalletButton)`
  font: ${leo.font.small.semibold};
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  padding: 3px 14px;
  letter-spacing: 0.01em;
  color: ${leo.color.neutral[70]};
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: 48px;
`

export const BackIcon = styled(CaratStrongLeftIcon)`
  width: auto;
  height: 14px;
  margin-right: 8px;
  color: ${leo.color.text.tertiary};
`
