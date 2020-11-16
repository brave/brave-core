/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
`

export const header = styled.div`
  display: flex;
  padding: 13px 18px 5px 20px;
  border-radius: 8px 8px 0 0;
  color: var(--brave-palette-neutral900);
  background: rgba(255, 255, 255, 0.65);
`

export const title = styled.div`
  flex: 1 1 auto;
  color: var(--brave-palette-neutral900);
  font-weight: 600;
  font-size: 16px;
  line-height: 26px;
`

export const date = styled.div`
  flex: 1 1 auto;
  text-align: right;
  font-size: 14px;
  line-height: 24px;
  color: var(--brave-palette-grey700);
`

export const icon = styled.span`
  display: inline-block;
  height: 23px;
  width: 23px;
  vertical-align: middle;
  margin-bottom: 3px;
  margin-left: 5px;
`

export const text = styled.div`
  overflow: hidden;
  padding: 19px 21px;
  text-overflow: ellipsis;
  white-space: pre-wrap;
  background: var(--brave-palette-white);
  border-radius: 0 0 8px 8px;
  color: var(--brave-palette-black);
`
