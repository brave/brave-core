// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icons
import SwitchDown from '../../../assets/svg-icons/switch-icon.svg'

// styles
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  margin-bottom: 18px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountAndAddress = styled(WalletButton)`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const AccountName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const AccountAddress = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`
export const AccountCircle = styled(WalletButton)<{
  orb: string
}>`
  display: flex;
  cursor: pointer;
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin: 0px;
  padding: 0px;
  outline: none;
  border: none;
  margin-right: 8px;
  position: relative;
`

export const SwitchIcon = styled.div`
  width: 14px;
  height: 14px;
  background: url(${SwitchDown});
  position: absolute;
  left: -2px;
  bottom: -2px;
  z-index: 8;
`
