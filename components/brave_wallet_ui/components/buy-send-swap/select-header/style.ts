// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'
import Plus from '../../../assets/svg-icons/plus-icon.svg'

export const Header = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
`

export const HeaderText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const Button = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 16px;
  height: 16px;
  padding: 0px;
`

export const BackIcon = styled(CaratStrongLeftIcon)`
  width: 16px;
  height: 16px;
  color: ${(p) => p.theme.color.text02};
`

export const HeaderSpacing = styled.div`
  width: 16px;
  height: 16px;
`

export const PlusIcon = styled.div`
  width: 15px;
  height: 15px;
  background: url(${Plus});
  color: ${(p) => p.theme.color.text02};
`
