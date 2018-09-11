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
  width: 90px;
`

export const ImportImage = styled(BaseImage)`
  width: 215px;
`

export const SearchImage = styled(BaseImage)`
  width: 394px;
`

export const ThemeImage = styled(BaseImage)`
  width: 297px;
`

export const PaymentsImage = styled(BaseImage)`
  width: 230px;
`

export const ShieldsImage = styled(BaseImage)`
  width: 185px;
`
