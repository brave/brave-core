// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

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
    border-radius: var(--leo-radius-8) 0 0 var(--leo-radius-8);
  }

  &:last-child {
    border-radius: 0 var(--leo-radius-8) var(--leo-radius-8) 0;
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
  border-radius: var(--leo-radius-8);
  border: 0;
  cursor: pointer;
  align-items: center;
  background-color: transparent;

  &.is-active {
    color: rgb(77, 82, 253);
    background-color: var(--color-button-active);
  }

  &:enabled {
    &:hover {
      background-color: var(--color-button-hover);
    }
  }

  ${props => props?.inGroup && GroupStyles}
`
