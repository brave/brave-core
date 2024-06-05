// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { CSSProperties } from 'styled-components'
import { color } from '@brave/leo/tokens/css/variables'

export const CollectionGrid = styled.div`
  display: grid;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr 1fr;
  grid-gap: 8px 8px;
  & > * {
    position: relative;
    border-radius: 8px;
    width: 100%;
    height: 100%;
    max-height: 100px;
    overflow: hidden;
  }
`

export const EmptyCollectionGridItem = styled.div`
  background: ${color.page.background};
`

export const NftIconStyles: CSSProperties = {
  borderRadius: 8,
  width: '100%',
  height: 'auto'
}
