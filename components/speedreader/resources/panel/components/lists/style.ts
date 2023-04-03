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

export const Button = styled.button`
  display: flex;
  width: 28px;
  height: 28px;
  padding: 0;
  justify-content: center;
  border-radius: 4px;
  border: 0;
  cursor: pointer;
  align-items: center;
  background-color: transparent;

  &.group {
    width: 32px;

    border-radius: 0;
    border-style: solid;
    border-color: var(--color-border);

    border-left-width: 0;
    border-top-width: 1px;
    border-bottom-width: 1px;
    border-right-width: 1px;
  }

  &.group:first-of-type {
    border-left-width: 1px;
    border-radius: 4px 0 0 4px;
  }

  &.group:last-of-type {
    border-radius: 0 4px 4px 0;
  }

  &.is-active {
    background-color: rgba(19, 22, 32, 0.08);
  }

  &:enabled {
    &:hover {
      background-color: rgba(19, 22, 32, 0.05);
    }
  }
`

export const CurrentState = styled(Button)`
  gap: 8px;
  width: 78px !important;
  font-size: 12px;
  line-height: 14px;
  font-weight: 400;
  font-style: normal;
  letter-spacing: -0.289412px;
`