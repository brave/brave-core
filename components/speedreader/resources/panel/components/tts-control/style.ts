// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { radius } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  display: flex;
  justify-content: flex-end;
  align-items: center;
  width: 100%;
  gap: 8px;
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

export const Voice = styled.div`
  display: flex;
  height: 100%;
  align-items: center;
  justify-items: center;

  select {
    margin-left: 8px;

    width: 112px;
    padding: 4px;
    height: 28px;

    background: var(--color-background);
    color: var(--color-foreground);

    border: 1px solid var(--color-button-border);
    border-radius: ${radius.s};

    &:focus {
      outline: none;
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
