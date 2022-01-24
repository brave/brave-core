// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { Button } from 'brave-ui'

export const Section = styled('div')`
  height: 100%;
  overscroll-behavior: contain;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: stretch;
`

export const StaticPrefs = styled('div')`
  flex: 0 0 auto;
`

export const PublisherList = styled('div')`
  flex: 1 1 0;
  overscroll-behavior: contain;
`
export const PublisherListScrollViewStyle: React.CSSProperties = {
  overscrollBehavior: 'contain'
}

export const SourcesCommandIcon = styled('div')`
  width: 14px;
  height: 14px;
`

export const PublisherListItem = styled('div')`
  padding: 0 12px 0 0;
`

export const FeedInputLabel = styled('label')`
  margin-top: 8px;
  font-weight: 500;
  font-size: 13px;
  line-height: 16px;
  color: #495057;
  @media (prefers-color-scheme: dark) {
    color: #C2C4CF;
  }
`

export const FeedInput = styled('input')`
  outline: none;
  margin-top: 6px;
  width: 389px;
  height: 40px;
  padding: 10px 18px;
  border-radius: 4px;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 400;
  line-height: 20px;
  letter-spacing: 0.01em;
  text-align: left;

  background: white;
  border: 1px solid #AEB1C2;
  color: #495057;
  @media (prefers-color-scheme: dark) {
    background: #1E2029;
    border: 1px solid #5E6175;
    color: #C2C4CF;
  }

  &:focus, :hover {
    border: 4px solid #A0A5EB;
    padding: 7px 15px;
  }

  &::placeholder {
    color: #84889C;
  }
`

export const YourSources = styled('div')`
  margin-bottom: 16px;
  border-bottom: solid 1px #E6E8F5;
  padding-bottom: 16px;
  display: flex;
  flex-direction: column;
  align-items: stretch;
`

export const TemporaryFixedButton = styled(Button)`
  color: white;
  align-self: flex-end;
  margin-top: 6px;
`

export const FeedUrlError = styled('p')`
  font: 400 12px/18px Poppins;
  padding: 0;
  margin: 2px 0 0 0;
  color: #bd1531;
`
