// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  Block as StandardBlock,
  Heading as StandardHeading,
  Time as StandardTime,
  PublisherLogo as StandardPublisherLogo
} from '../../default'

export const OrderedList = styled(StandardBlock)``

export const ListTitle = styled('div')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: 500;
  font-size: 28px;
  color: #fff;
`

export const List = styled('ol')`
  box-sizing: border-box;
  counter-reset: item;
  margin-top: 36px;
  margin-left: 48px;
  padding: 0;
`

export const ListItem = styled('li')<{}>`
  position: relative;
  display: block;
  margin-bottom: 36px;
  color: #fff;

  &:before {
    position: absolute;
    left: -48px;
    height: 100%;
    content: counter(item) " ";
    counter-increment: item;
    display: marker;
    font-size: 18px;
    line-height: 25px;
  }
`

export const Heading = styled(StandardHeading)`
  font-size: 18px;
  line-height: 25px;

  a {
    display: block;
    color: inherit;
    text-decoration: none;
  }
`

export const Time = styled(StandardTime)``

export const PublisherLogo = styled(StandardPublisherLogo)`
  height: 50px;
`
