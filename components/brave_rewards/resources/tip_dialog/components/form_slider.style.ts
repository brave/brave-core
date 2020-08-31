/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  overflow: hidden;
  height: 100%;
`

export const track = styled.div`
  display: flex;
  transition: margin-left .3s ease;
  height: 100%;

  > * {
    width: 100%;
  }
`
