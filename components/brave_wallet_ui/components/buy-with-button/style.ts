/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import CaratDownIcon from '../../assets/svg-icons/select-down-icon.svg'
import { WalletButton } from '../shared/style'

export interface OptionProps {
  selected?: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  position: relative;
  box-sizing: border-box;
  width: 100%;
  `

export const Button = styled(WalletButton)`
  display: flex;
  align-items: center;
  width: 100%;
  border: none;
  color: ${(p) => p.theme.color.text02};
  font-family: Poppins;
  font-weight: 400;
  box-sizing: border-box;
  background-color: ${(p) => p.theme.color.background02};
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  padding: 9px 12px;
  border-radius: 12px;
  cursor: ${p => p.disabled ? 'auto' : 'pointer'};
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
`

export const Dropdown = styled.ul`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  box-shadow: 0 0 16px rgba(99, 105, 110, 0.18);
  border-radius: 4px;
  width: 100%;
  background-color: ${(p) => p.theme.color.background02};
  padding: 10px 12px;
  margin: 0;
  position: absolute;
  top: calc(100% + 2px);
  z-index: 3;
  box-sizing: border-box;
`

export const CaratDown = styled.div`
  width: 12px;
  height: 6px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CaratDownIcon});
  mask-image: url(${CaratDownIcon});
  position: absolute;
  right: 12px;
`

export const Option = styled.li<Partial<OptionProps>>`
  display: flex;
  align-items: center;
  padding: 10px 0;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: ${(p) => p.selected ? 600 : 400};
  cursor: pointer;
`
