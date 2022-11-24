// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import CloseIcon from '../../../assets/svg-icons/close.svg'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  hasSearch: boolean
}

export const HeaderTitle = styled.span`
  font-family: Poppins;
  font-size: 18px;
  line-height: 26px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`

export const HeaderWrapper = styled.div<StyleProps>`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border-bottom: ${(p) => `1px solid ${p.theme.color.divider01}`};
  padding: 0px 12px;
  max-width: 300px;
  margin-bottom: ${(p) => p.hasSearch ? '0px' : '8px'};
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
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${CloseIcon});
  outline: none;
  border: none;
`
