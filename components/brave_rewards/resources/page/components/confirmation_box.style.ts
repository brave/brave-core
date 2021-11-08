/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  max-width: var(--confirmation-box-max-width, 377px);
  background: var(--brave-palette-white);
  border-radius: 13px;
  padding: 32px 25px;
  text-align: center;
  color: var(--brave-palette-neutral700);
`

export const header = styled.div`
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  var(--brave-palette-neutral700);
`

export const text = styled.div`
  margin-top: 16px;
  font-weight: 500;
  font-size: 14px;
  line-height: 24px;
`

export const actions = styled.div`
  margin-top: 28px;
`
