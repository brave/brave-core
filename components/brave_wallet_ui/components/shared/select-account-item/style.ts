// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../style'
import CheckMark from '../../../assets/svg-icons/big-checkmark.svg'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled(WalletButton)<{
  isV2?: boolean
}>`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin-bottom: 10px;
  padding: 0px;
  background-color: ${(p) =>
    p.isV2 ? leo.color.container.highlight : 'transparent'};
  border-radius: ${(p) => (p.isV2 ? '8px' : 0)};
  padding: ${(p) => (p.isV2 ? '8px' : 0)};
`

export const AccountAndAddress = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const AccountName = styled.span<{
  isV2?: boolean
}>`
  font-family: Poppins;
  font-size: ${(p) => (p.isV2 ? '14px' : '13px')};
  line-height: ${(p) => (p.isV2 ? '24px' : '20px')};
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const AccountAddress = styled.span<{
  isV2?: boolean
}>`
  font-family: Poppins;
  font-size: ${(p) => (p.isV2 ? '11px' : '12px')};
  line-height: ${(p) => (p.isV2 ? '16px' : '18px')};
  color: ${leo.color.text.secondary};
`

export const AccountCircle = styled.div<StyleProps>`
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 8px;
`

export const BigCheckMark = styled.div`
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.color.text01};
  -webkit-mask-image: url(${CheckMark});
  mask-image: url(${CheckMark});
  margin-right: 8px;
`

export const LeftSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  margin-right: 6px;
`

export const SwitchAccountIconContainer = styled.div`
  padding-left: 8;
  padding-right: 8;
`

export const CaratDown = styled(Icon).attrs({
  name: 'carat-down'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
