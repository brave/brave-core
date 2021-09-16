/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import styled from 'styled-components'

export const TourPromoWrapper = styled('div')<{}>`
  margin-top: 30px;
`

export const PageWalletWrapper = styled.div`
  width: 373px;
`

export const ArrivingSoon = styled.div`
  background: rgba(93, 181, 252, 0.2);
  border-radius: 4px;
  margin-bottom: 8px;
  padding: 4px;
  color: var(--brave-palette-neutral700);
  text-align: center;
  font-size: 12px;
  line-height: 22px;

  span.amount {
    color: var(--brave-palette-neutral900);
    font-weight: 600;
    font-size: 14px;
    line-height: 24px;
  }

  .icon {
    height: 16px;
    width: auto;
    fill: var(--brave-palette-blue500);
    vertical-align: middle;
    margin-bottom: 3px;
    margin-right: 8px;
  }
`
