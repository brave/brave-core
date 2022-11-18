// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { WalletButton } from '../../../shared/style'

export const Button = styled(WalletButton)`
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  outline: none;
  border: none;
  background: none;
  pointer-events: auto;
  border-right: 1px solid rgba(255,255,255,0.5);
  height: 100%;
  box-sizing: border-box;
  cursor: pointer;
  position: relative;
  &:last-child { 
    border-right: none;
  }
`

export const ButtonIcon = styled.div <{ icon: string }>`
  -webkit-mask-image: url(${(p) => p.icon});
  mask-image: url(${(p) => p.icon});
  mask-size: contain;
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
`

export const Tip = styled.div<{ position: 'left' | 'right' | 'center' }>`
  position: absolute;
  border-radius: 4px;
  left: ${(p) => p.position === 'left' ? '0px' : 'unset'};
  right: ${(p) => p.position === 'right' ? '0px' : 'unset'};
  transform: translateY(-52px);
  padding: 8px 16px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 10;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 500;
`

export const Pointer = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  transform: translatey(-30px) rotate(180deg);
  border-width: 0 7px 8px 7px;
  z-index: 10;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`
