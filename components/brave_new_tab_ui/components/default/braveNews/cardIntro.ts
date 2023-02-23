// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  Block as StandardBlock,
  Image as StandardImage
} from './default'

export const Intro = styled(StandardBlock)`
  font-family: Poppins;
  color: white;
  text-align: center;
  padding: 44px 82px 36px;
  display: flex;
  flex-direction: column;
  gap: 30px;
  align-items: center;
`

export const Title = styled('h2')`
  // Negative bottom margin counteracts line-spacing interfering
  // with container flex gap.
  margin: 0 0 -5px 0;
  font-weight: 600;
  font-size: 28px;
  line-height: 1.2;
`

export const Paragraph = styled('p')`
  margin: 0;
  padding: 0;
  font-weight: 500;
  font-size: 14px;
  line-height: 17px;
  letter-spacing: 0.16px;

  & + & {
    margin-top: 10px;
  }

  a {
    color: inherit;
    text-decoration: underline;
  }
`

export const Image = styled(StandardImage)`
  margin: auto;
  height: 60px;
`
