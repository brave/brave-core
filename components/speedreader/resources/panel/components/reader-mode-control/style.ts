// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { radius } from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  align-items: center;
  width: 100%;
`

export const Caption = styled.div`
  display: flex;
  flex: 1 1 100%;
  font-weight: 400;
  font-size: 13px;
  line-height: 16px;
  justify-content: center;
  align-items: center;
  letter-spacing: -0.08px;
  height: 40px;
  gap: 8px;
`

export const CloseButton = styled.button`
  display: flex;
  flex: 1 1 28px;
  width: 28px;
  height: 28px;
  padding: 4px;
  justify-content: center;
  border-radius: ${radius.m};
  border: 0;
  cursor: pointer;
  align-items: center;

  &:hover {
    background-color: var(--color-button-hover);
  }

  background-color: transparent;
`
