// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

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
