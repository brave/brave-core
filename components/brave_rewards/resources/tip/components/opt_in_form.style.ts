/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const topBar = styled.div`
  flex: 0 0 auto;
  text-align: center;
  background: var(--brave-palette-grey200);
  color: var(--brave-palette-neutral600);
  font-weight: 600;
  font-size: 14px;
  line-height: 18px;
  padding: 17px 36px;
`

export const sadIcon = styled.span`
  display: inline-block;
  height: 22px;
  width: 22px;
  vertical-align: middle;
  margin-bottom: 1px;
  margin-right: 7px;
`

export const content = styled.div`
  flex: 1 0 auto;
`

export const batIcon = styled.div`
  height: 47px;
`
