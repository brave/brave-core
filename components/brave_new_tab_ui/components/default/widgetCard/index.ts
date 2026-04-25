/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'

export const StyledTitleTab = styled.div`
  color: ${color.white};
  font: ${font.heading.h4};
  cursor: pointer;
  padding: 16px 24px 24px 24px;
  margin-bottom: -16px;
  backdrop-filter: blur(20px);
  border-radius: 16px 16px 0 0;
  background: #1C1E2680;
  display: flex;
  align-items: center;
  gap: 8px;
`

export const StyledCard = styled.div`
  background: #1C1E26B2;
  backdrop-filter: blur(20px);
  border-radius: 16px;
  padding: 24px;

  &::before {
    content: "";
    position: absolute;
    z-index: -1;
    inset: 0;
    border-radius: 16px;
    padding: 1px;
    background: linear-gradient(
      156.52deg,
      rgba(0, 0, 0, 0.05) 2.12%,
      rgba(0, 0, 0, 0) 39%,
      rgba(0, 0, 0, 0) 54.33%,
      rgba(0, 0, 0, 0.15) 93.02%);
    mask:
      linear-gradient(#fff 0 0) content-box,
      linear-gradient(#fff 0 0);
    mask-composite: exclude;
  }
`
