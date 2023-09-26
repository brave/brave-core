// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const StyledWrapper = styled.div`
  display: flex;
  padding: ${leo.spacing.l} ${leo.spacing.m};
  justify-content: flex-start;
  align-items: flex-start;
  gap: ${leo.spacing.m};
  align-self: stretch;
  border-radius: ${leo.radius.m};
  cursor: pointer;

  &:hover {
    background-color: ${leo.color.container.highlight};
  }
`

export const DappImage = styled.img`
  width: 40px;
  height: 40px;
`

export const ContentWrapper = styled.div`
  display: inline-grid;
  text-align: left;
`

export const Name = styled.span`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
`

export const Description = styled.span`
  color: ${leo.color.text.tertiary};
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 400;
  line-height: 18px;
  min-width: 0;
`
