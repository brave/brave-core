// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import CloseIcon from '../../../assets/svg-icons/close.svg'

// Shared Styles
import { WalletButton } from '../../shared/style'

interface StyleProps {
  hasSearch: boolean
}

export const HeaderWrapper = styled.div<StyleProps>`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border-bottom: 1px solid ${leo.color.divider.subtle};
  padding: 0px 12px;
  margin-bottom: ${(p) => (p.hasSearch ? '0px' : '8px')};
`

export const TopRow = styled.div`
  display: flex;
  height: 54px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
`

export const CloseButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${CloseIcon});
  outline: none;
  border: none;
`
