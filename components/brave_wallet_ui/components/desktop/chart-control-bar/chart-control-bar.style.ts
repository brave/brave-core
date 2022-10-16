// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  box-sizing: border-box;
  border-radius: 12px;
  padding: 8px;
  --selected-color: ${p => p.theme.palette.white};
  @media (prefers-color-scheme: dark) {
    --selected-color: ${p => p.theme.color.background02};
  }
`

export const StyledButton = styled(WalletButton)<{ isSelected?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: ${(p) => p.disabled ? 'not-allowed' : 'pointer'};
  border-radius: 4px;
  outline: none;
  padding: 4px 6px;
  background: ${(p) =>
    p.isSelected && !p.disabled ? p.theme.color.text02 : 'none'};
  border: none;
  margin: 0px 2px;
`

export const ButtonText = styled.span<{ isSelected?: boolean, disabled?: boolean }>`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${p => p.disabled
    ? p.theme.color.disabled
    : p.isSelected
      ? 'var(--selected-color)'
      : p.theme.color.text02
  };
`

export const Dot = styled.div<{ isSelected?: boolean }>`
  width: 6px;
  height: 6px;
  border-radius: 100%;
  margin-right: 6px;
  background-color: ${p => p.isSelected ? 'var(--selected-color)' : p.theme.color.disabled};
`
