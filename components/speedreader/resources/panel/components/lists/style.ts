// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { radius } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  display: flex;
  height: 100%;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  background-color: transparent;
`

const GroupStyles = `
  width: 32px;

  border-radius: 0;
  border-style: solid;
  border-color: var(--color-button-border);

  border-left-width: 0;
  border-top-width: 1px;
  border-bottom-width: 1px;
  border-right-width: 1px;

  &:first-child {
    border-left-width: 1px;
    border-radius: ${radius.m} 0 0 ${radius.m};
  }

  &:last-child {
    border-radius: 0 ${radius.m} ${radius.m} 0;
  }
`

export const CurrentStateIndicator = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  background-color: transparent;
  gap: 8px;
  min-width: 78px;
  height: 28px;
  font-size: 12px;
  font-weight: 400;
  font-style: normal;
  line-height: 14px;
  letter-spacing: -0.289412px;

  ${GroupStyles}
`

export const Button = styled.button<{inGroup?: boolean}>`
  display: flex;
  width: 28px;
  height: 28px;
  padding: 0;
  justify-content: center;
  border-radius: ${radius.m};
  border: 0;
  cursor: pointer;
  align-items: center;
  background-color: transparent;

  &.is-active {
    color: var(--color-button-active-text);
    background-color: var(--color-button-active);
  }

  &:enabled {
    &:hover {
      background-color: var(--color-button-hover);
    }
  }

  ${props => props?.inGroup && GroupStyles}
`
