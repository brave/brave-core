/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  --self-width: var(--tooltip-width, 280px);
  --self-center: calc(50% - var(--self-width) / 2);
  --self-offset: var(--tooltip-offset, 0px);

  position: absolute;
  bottom: 10px;
  width: var(--self-width);
  left: calc(var(--self-center) + var(--self-offset));
  right: calc(var(--self-center) + var(--self-offset));
  padding-bottom: 20px;
`

export const content = styled.div`
  position: relative;
  background: ${leo.color.container.background};
  box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);
  border-radius: 8px;
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 8px;
`

export const arrow = styled.div`
  background: ${leo.color.container.background};
  transform: rotate(45deg);
  width: 10px;
  height: 10px;
  position: absolute;
  bottom: -5px;
  left: calc(50% - 5px - var(--self-offset));
  right: calc(50% - 5px - var(--self-offset));
`
