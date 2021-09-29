/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: rgba(93, 181, 252, 0.2);
  border-radius: 4px;
  margin-bottom: 8px;
  padding: 4px;
  color: var(--brave-palette-neutral700);
  line-height: 1.7;
`

export const body = styled.div`
  font-size: 16px;
  font-weight: 800;

  .icon {
    height: 16px;
    width: auto;
    fill: var(--brave-palette-blue500);
    vertical-align: middle;
    margin-left: 8px;
    margin-right: 8px;
  }
`

export const note = styled.div`
  font-size: 14px;
  font-weight: 400;
  margin-left: 32px;
  color: var(--brave-palette-grey700);
`
