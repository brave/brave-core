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
} from '../../default'

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
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    align-items: center;
    gap: 50px;
  }
`

export const Content = styled('div')<{}>`
  height: auto;
  display: flex;
  flex-direction: column;
  justify-content: center;
`

export const ListItemImageFrame = styled('div')`
  flex: 0 0 100px;
  height: 100px;
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

  a {
    display: block;
    color: inherit;
    text-decoration: none;
  }
`

export const Publisher = styled('div')`
  max-width: 100%;
  display: inline-block;
  overflow: hidden;
  text-overflow: ellipsis;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  color: white;
  font-weight: 500;
`

export const Time = styled(StandardTime)``

export const PublisherLogo = styled(StandardPublisherLogo)``
