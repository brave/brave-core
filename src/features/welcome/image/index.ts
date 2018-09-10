/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const baseImage = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  max-width: 100%;
`

export const Brave = styled(baseImage)`
  width: 90px;
`

export const Payments = styled(baseImage)`
  width: 230px;
`

export const Import = styled(baseImage)`
  width: 215px;
`

export const Shields = styled(baseImage)`
  width: 185px;
`

export const Features = styled(baseImage)`
  width: 300px;
`
