/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div``

export const pendingRewards = styled.div`
  background: var(--brave-palette-grey900);
  color: var(--brave-palette-grey400);

  .brave-theme-dark & {
    background: rgba(30, 32, 41, .8);
  }

  font-size: 12px;
  line-height: 22px;
  text-align: center;
  padding: 8px;

  .icon {
    color: var(--brave-palette-yellow500);
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-right: 7px;
    margin-bottom: 3px;
  }
`

export const pendingAmount = styled.span`
  color: var(--brave-palette-grey000);
  font-size: 14px;
  line-height: 24px;

  .plus {
    margin-right: 2px;
  }
`
