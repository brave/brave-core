/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const main = styled.div`
  flex: 1 1 auto;
  padding: 0 32px;
  align-self: center;
  width: 100%;
  max-width: 450px;
`

export const amounts = styled.div`
  padding-top: 24px;
  margin-top: 11px;
  border-top: 1px solid rgba(174, 177, 194, 0.5);
}
`

export const footer = styled.div``

export const addFunds = styled.div`
  padding: 19px 0;
  text-align: center;
  color: var(--brave-palette-white);
  background: var(--brave-palette-neutral600);
  font-size: 14px;
  line-height: 21px;
  font-weight: 600;

  a {
    color: var(--brave-color-brandBatInteracting);
  }
`

export const sadIcon = styled.span`
  display: inline-block;
  height: 22px;
  width: 22px;
  vertical-align: middle;
  margin: -1.5px 2px 0 0;
  color: var(--brave-palette-neutral300);
`
