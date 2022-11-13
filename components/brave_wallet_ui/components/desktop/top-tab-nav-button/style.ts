// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  isSelected: boolean
  icon: string
}

export const StyledButton = styled(WalletButton) <Partial<StyleProps>>`
  display: flex;
  width: 140px;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  padding: 10px 0px 0px 0px;
  border: none;
  background: none;
`

export const ButtonText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 0.04em;
  line-height: 20px;
  margin-bottom: 10px;
  background: ${(p) =>
    p.isSelected ? p.theme.color.text01 : p.theme.color.text02};
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
`

export const TabLine = styled.div<Partial<StyleProps>>`
  display: flex;
  width: 100%;
  height: 2px;
  background: ${(p) =>
    p.isSelected ? 'linear-gradient(178.53deg, #4C54D2 0%, #BF14A2 56.25%, #F73A1C 100%);' : `${p.theme.color.divider01}`};
`
