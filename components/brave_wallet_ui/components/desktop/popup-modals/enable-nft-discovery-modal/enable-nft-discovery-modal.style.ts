// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const Header = styled.h1`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  color: ${leo.color.text.primary};
  margin: 0 0 8px 0;
  padding: 0;
  text-align: left;
  width: 100%;
`

export const Description = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
  margin: 0;
  padding: 0;
`

export const Link = styled.a`
  color: ${leo.color.button.background};
  text-decoration: none;
  font-weight: 600;
`

export const Underline = styled.span`
  text-decoration: underline;
`

export const ButtonRow = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  margin-top: 24px;
  gap: ${leo.spacing.l};
  flex-wrap: wrap-reverse;
`
