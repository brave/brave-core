// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

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
  height: 36px;
  border: none;
  color: ${(p) => p.theme.color.text01};
  font-family: Poppins;
  box-sizing: border-box;
  background-color: ${(p) => p.theme.color.background02};
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  padding: 6px 12px;
  border-radius: 8px;
  cursor: pointer;
`

export const Dropdown = styled.ul`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  border-radius: 4px;
  width: 100%;
  background-color: ${(p) => p.theme.color.background02};
  padding: 10px 12px;
  margin: 0;
  position: absolute;
  top: calc(100% + 2px);
  z-index: 3;
`

export const CaratDown = styled.div`
  width: 16px;
  height: 16px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CaratDownIcon});
  mask-image: url(${CaratDownIcon});
`
