// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { WalletButton } from '../../../shared/style'

export const StyledButton = styled(WalletButton)`
  --button-color: ${(p) => p.theme.color.text02};
  --button-background: ${(p) => p.theme.color.background02};
  &:hover {
    --button-color: ${(p) => p.theme.color.text01};
    --button-background: ${(p) => p.theme.color.divider01};
  }
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  cursor: pointer;
  padding: 18px;
  outline: none;
  border: none;
  background-color: var(--button-background);
  border-bottom: 1px solid ${(p) => p.theme.color.divider01};
  color: var(--button-color);
  font-weight: 600;
  font-size: 16px;
  font-family: 'Poppins';
  &:last-child { 
    border-bottom: none;
  }
`

export const ButtonIcon = styled.div <{ icon: string }>`
  -webkit-mask-image: url(${(p) => p.icon});
  mask-image: url(${(p) => p.icon});
  mask-size: contain;
  width: 20px;
  height: 20px;
  background-color: var(--button-color);
  margin-right: 10px;
`
