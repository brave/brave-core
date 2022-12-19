// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { WalletButton } from '../../../shared/style'

export const StyledButton = styled(WalletButton) <{ isTab?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  cursor: pointer;
  padding: ${(p) => p.isTab ? 10 : 18}px;
  outline: none;
  border: none;
  background-color: var(--nav-button-background);
  border-bottom: ${(p) => p.isTab ? 'none' : `1px solid ${p.theme.color.divider01}`};
  border-radius: ${(p) => p.isTab ? 6 : 0}px;
  margin-bottom: ${(p) => p.isTab ? 16 : 0}px;
  color: var(--nav-button-color);
  font-weight: 600;
  font-size: 16px;
  font-family: 'Poppins';
  &:hover {
    color: var(--nav-button-color-hover);
    background-color: var(--nav-button-background-hover);
  }
  &:last-child { 
    border-bottom: none;
    margin-bottom: 0px;
  }
`

export const ButtonIcon = styled.div <{ icon: string, isTab?: boolean }>`
  -webkit-mask-image: url(${(p) => p.icon});
  mask-image: url(${(p) => p.icon});
  mask-size: contain;
  width: 20px;
  height: 20px;
  background-color: var(--nav-button-color);
  margin-right: ${(p) => p.isTab ? 0 : 10}px;
`
