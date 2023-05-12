// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Text } from '../../../shared/style'
import { layoutSmallWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledButton = styled(WalletButton) <{ isSelected?: boolean }>`
  --icon-color: ${(p) =>
    p.isSelected
      ? 'var(--nav-page-icon-color-hover)'
      : 'var(--nav-page-icon-color)'
  };
  --text-color: ${(p) =>
    p.isSelected
      ? 'var(--nav-page-text-color-hover)'
      : 'var(--nav-page-text-color)'
  };
  --indicator-color: ${(p) =>
    p.isSelected
      ? 'var(--nav-page-indicator-color)'
      : 'none'
  };
  &:hover {
    --icon-color: var(--nav-page-icon-color-hover);
    --text-color: var(--nav-page-text-color-hover);
    }
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  cursor: pointer;
  padding: 6px 0px;
  outline: none;
  border: none;
  background: none;
  margin-bottom: 12px;
  &:last-child {
    margin-bottom: 0px;
  }
  @media screen and (max-width: ${layoutSmallWidth}px) {
    border-radius: 6px;
    padding: 6px 0px 3px 0px;
    width: 100%;
    flex-direction: column;
    margin-bottom: 0px;
    margin-right: 8px;
    background-color: ${(p) => p.isSelected
    ? 'var(--nav-button-background-hover)'
    : 'none'};
    &:hover {
      background-color: var(--nav-button-background-hover);
    }
    &:last-child {
      margin-right: 0px;
    }
  }
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: var(--icon-color);
  margin-right: 16px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    color: var(--nav-button-color);
    --leo-icon-size: 24px;
    margin-right: 0px;
  }
`

export const ButtonText = styled(Text)`
  color: var(--text-color);
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    color: var(--nav-button-color);
    font-size: 12px;
    font-weight: 400;
    line-height: 18px;
  }
`

export const SelectedIndicator = styled.div`
  display: flex;
  width: 4px;
  height: 32px;
  margin-right: 20px;
  background-color: var(--indicator-color);
  border-radius: 0px 2px 2px 0px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    display: none;
  }
`
