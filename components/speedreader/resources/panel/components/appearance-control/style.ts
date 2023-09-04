// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  justify-content: flex-end;
  align-items: center;
  width: 100%;
  gap: 6px;
  padding-right: 8px;

  @media screen and (min-width: 600px) {
    gap: 16px;

    & > *:not(:last-child)::after {
      content: '';
      margin-left: 16px;
      width: 1px;
      height: 20px;
      background: var(--color-button-border);
    }
  }
`

export const CloseButton = styled.button`
  background: transparent;
  height: 28px;
  padding: 0;
  justify-content: center;
  border: 0;
  cursor: pointer;
  font-size: 400;

  @media screen and (max-width: 600px) {
    display: none;
  }
`