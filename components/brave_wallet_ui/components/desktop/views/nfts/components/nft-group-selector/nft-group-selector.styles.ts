// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

import { WalletButton } from '../../../../../shared/style'

export const DropdownContainer = styled.div`
  position: relative;
  display: inline-block;
`

export const DropdownButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: transparent;
  padding: 8px 12px;
  gap: ${leo.spacing.m};
  border: none;
  outline: transparent;
  cursor: pointer;
`

export const DropdownButtonText = styled.span`
  color: ${leo.color.text.interactive};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 22px;
`

export const DropdownButtonLabel = styled.span`
  display: flex;
  padding: 4px;
  align-items: center;
  justify-content: center;
  color: ${leo.color.primary[50]};
  font-family: Poppins;
  font-size: 11px;
  font-style: normal;
  font-weight: 600;
  text-transform: uppercase;
  background-color: ${leo.color.primary[20]};
  border-radius: 4px;
  min-width: 20px;
`

export const DropDownIcon = styled(Icon)<{ isOpen: boolean }>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.interactive};
  transition-duration: 0.3s;
  transform: ${(p) => (p.isOpen ? 'rotate(180deg)' : 'unset')};
`

export const DropDown = styled.div<{ isOpen: boolean }>`
  display: flex;
  position: absolute;
  top: 100%;
  left: 0;
  width: 147px;
  padding: ${leo.spacing.s};
  flex-direction: column;
  align-items: flex-start;
  gap: ${leo.spacing.s};
  border-radius: ${leo.spacing.m};
  border: 1px solid ${leo.color.divider.subtle};
  background-color: ${leo.color.container.background};
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
  display: ${({ isOpen }) => (isOpen ? 'flex' : 'none')};
  transition: opacity 0.3s, transform 0.3s;
  opacity: ${({ isOpen }) => (isOpen ? 1 : 0)};
  transform: translateY(${({ isOpen }) => (isOpen ? '0' : '-10px')});
  z-index: 2;
`

export const DropDownItem = styled(WalletButton)`
  display: flex;
  height: 44px;
  padding: 0px 8px;
  align-items: center;
  gap: 8px;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 24px;
  color: ${leo.color.text.primary};
  cursor: pointer;
  border: none;
  background-color: transparent;
`
