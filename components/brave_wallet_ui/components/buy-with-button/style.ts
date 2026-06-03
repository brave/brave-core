/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as leo from '@brave/leo/tokens/css/variables'
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
  color: ${leo.color.text.secondary};
  font: ${leo.font.default.regular};
  box-sizing: border-box;
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.divider.subtle};
  padding: 9px 12px;
  border-radius: 12px;
  cursor: ${(p) => (p.disabled ? 'auto' : 'pointer')};
  letter-spacing: 0.01em;
`

export const Dropdown = styled.ul`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  box-shadow: 0 0 16px rgba(99, 105, 110, 0.18);
  border-radius: 4px;
  width: 100%;
  background-color: ${leo.color.container.background};
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
  background-color: ${leo.color.text.secondary};
  -webkit-mask-image: url(${CaratDownIcon});
  mask-image: url(${CaratDownIcon});
  position: absolute;
  right: 12px;
`

export const Option = styled.li<Partial<OptionProps>>`
  display: flex;
  align-items: center;
  padding: 10px 0;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${leo.color.text.secondary};
  font-weight: ${(p) => (p.selected ? 600 : 400)};
  cursor: pointer;
`
