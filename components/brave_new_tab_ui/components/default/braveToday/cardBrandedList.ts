// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  Block as StandardBlock,
  Heading as StandardHeading,
  Image as StandardImage,
  Time as StandardTime,
  PublisherLogo as StandardPublisherLogo
} from './default'

export const BrandedList = styled(StandardBlock)``

export const List = styled('ul')`
  box-sizing: border-box;
  padding: 0;
  margin: 0;
  list-style: none;
`

export const ListItem = styled('li')<{}>`
  margin-bottom: 24px;

  a {
    text-decoration: none;
    display: grid;
    grid-template-columns: 1fr auto;
    grid-gap: 56px;
  }
`

export const Content = styled('div')<{}>`
  height: auto;
  display: flex;
  flex-direction: column;
  justify-content: center;
`

export const Image = styled(StandardImage)`
  margin: auto;
  object-fit: cover;
  width: 100px;
  height: 100px;
  border-radius: 8px;
  background-color: rgba(188,188,188,0.2);
`

export const Title = styled(StandardHeading)`
  margin-bottom: 16px;
`

export const Heading = styled(StandardHeading)`
  font-size: 18px;
  line-height: 25px;
  font-weight: normal;

  a {
    display: block;
    color: inherit;
    text-decoration: none;
  }
`

export const Time = styled(StandardTime)``

export const PublisherLogo = styled(StandardPublisherLogo)``
