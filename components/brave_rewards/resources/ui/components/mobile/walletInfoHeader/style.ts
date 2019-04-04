/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'
import panelBgUrl from './assets/panel.svg'

export const StyledWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  border-radius: 6px;
  margin-bottom: 15px;
  font-family: ${p => p.theme.fontFamily.heading};
  background: url(${panelBgUrl}) no-repeat top left,
  linear-gradient(to bottom right, #392dd1 0%, #5813a6 100%) 100% no-repeat;
  flex-direction: column;
  box-shadow: 0 2px 4px rgba(0,0,0,.2);
`

export const StyledHeader = styled<{}, 'div'>('div')`
  padding: 16px 21px 14px 19px;
  position: relative;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-weight: 300;
  line-height: 1.38;
  letter-spacing: -0.2px;
  color: rgba(255, 255, 255, 0.65);

  @media (max-width: 360px) {
    font-size: 14px;
  }
`

export const StyledBalance = styled<{}, 'div'>('div')`
  margin-top: -14px;
  text-align: center;
`

export const StyledBalanceTokens = styled<{}, 'div'>('div')`
  font-size: 38px;
  line-height: 0.61;
  letter-spacing: -0.4px;
  color: #fff;
  font-weight: 300;
  margin-top: 10px;
`

export const StyledBalanceConverted = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.17;
  text-align: center;
  color: rgba(255, 255, 255, 0.65);
  margin: 8px 0;
`

export const StyledBalanceCurrency = styled<{}, 'span'>('span')`
  text-transform: uppercase;
  opacity: 0.66;
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 14px;
  color: #fff;
  letter-spacing: 0px;
`
