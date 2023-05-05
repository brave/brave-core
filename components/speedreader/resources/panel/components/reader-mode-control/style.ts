// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
`

export const Caption = styled.div`
  display: flex;
  font-weight: 400;
  font-size: 13px;
  line-height: 16px;
  justify-content: center;
  align-items: center;
  letter-spacing: -0.08px;
  height: 40px;
  gap: 8px;
`

export const Button = styled.button`
  display: flex;
  width: 28px;
  height: 28px;
  padding: 4px;
  justify-content: center;
  border-radius: 4px;
  border: 0;
  cursor: pointer;
  align-items: center;

  &:hover {
    background-color: rgba(0, 0, 0, 0.1);
  }

  background-color: transparent;
`
