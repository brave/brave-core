/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const BaseImage = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  max-width: 100%;
`

export const BraveImage = styled(BaseImage)`
  height: 140px;
`

export const ImportImage = styled(BaseImage)`
  height: 200px;
`

export const SearchImage = styled(BaseImage)`
  height: 200px;
`

export const ThemeImage = styled(BaseImage)`
  height: 200px;
`

export const PaymentsImage = styled(BaseImage)`
  height: 200px;
`

export const ShieldsImage = styled(BaseImage)`
  height: 200px;
`
