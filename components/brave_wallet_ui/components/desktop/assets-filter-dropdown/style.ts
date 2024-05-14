// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import CaratDownIcon from '../../../assets/svg-icons/carat-down.svg'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  position: relative;
  min-width: 134px;
  box-sizing: border-box;
`

export const Button = styled(WalletButton)`
  display: flex;
  width: 100%;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  height: 44px;
  border: none;
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 14px;
  letter-spacing: 0.01em;
  box-sizing: border-box;
  background-color: ${leo.color.container.highlight};
  padding: 6px 12px;
  border: none;
  border-radius: 8px;
  cursor: pointer;
`

export const Dropdown = styled.ul`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  margin: 0;
  padding: 8px 8px 0px 8px;
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.25);
  position: absolute;
  top: calc(100% + 2px);
  left: 2px;
  z-index: 3;
`

export const CaratDown = styled.div`
  width: 16px;
  height: 16px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CaratDownIcon});
  mask-image: url(${CaratDownIcon});
`
