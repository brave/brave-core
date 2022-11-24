// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

interface StyleProps {
  isSelected: boolean
  icon: string
}

export const StyledButton = styled.button<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  border-radius: 8px;
  width: 100%;
  outline: none;
  padding: 13px;
  background: ${(p) =>
    p.isSelected ? 'linear-gradient(128.18deg, #A43CE4 13.94%, #A72B6D 84.49%)' : 'none'};
  border: none;
`

export const ButtonText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) =>
    p.isSelected ? p.theme.palette.white : p.theme.color.text02};
`

export const ButtonIcon = styled.div<Partial<StyleProps>>`
  width: 18px;
  height: 18px;
  background: ${(p) => `url(${p.icon})`};
  margin-right: 10px;
`
