// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'

export const DappsGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  column-gap: ${leo.spacing['3Xl']};
  height: 100%;
`

export const CategoryHeader = styled.p`
  width: 100%;
  font: ${leo.font.heading.h4};
  color: ${leo.color.text.primary};
  text-align: left;
  text-transform: capitalize;
  padding: 0;
  margin: 0;
`

export const ShowMore = styled(Button).attrs({
    kind: 'plain'
})`
    --leo-button-color: ${leo.color.text.interactive};
    --leo-button-padding: 8px;
    width: 100%;
`
