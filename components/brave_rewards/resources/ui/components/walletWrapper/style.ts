/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyledProps {
  connected?: boolean
}

const panelBg = require('./assets/panel.svg')

export const StyledWrapper = styled<{}, 'div'>('div')`
  border-radius: 6px;
  box-shadow: 0 0 8px 0 rgba(99, 105, 110, 0.12);
  overflow: hidden;
  font-family: Poppins, sans-serif;
  width: 373px;
  background:
    url(${panelBg}) no-repeat top left,
    linear-gradient(172deg, #392dd1, rgba(255, 26, 26, 0.53)),
    linear-gradient(#7d7bdc, #7d7bdc);
  min-height: 715px;
  display: flex;
  flex-direction: column;
`

export const StyledHeader = styled<{}, 'div'>('div')`
  padding: 16px 21px 0 19px;
  position: relative;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-weight: 500;
  line-height: 1.38;
  letter-spacing: -0.2px;
  color: rgba(255, 255, 255, 0.65);
`

export const StyledBalance = styled<{}, 'div'>('div')`
  text-align: center;
`

export const StyleGrantButton = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
`

export const StyledBalanceTokens = styled<{}, 'div'>('div')`
  font-size: 36px;
  line-height: 0.61;
  letter-spacing: -0.4px;
  color: #fff;
  margin-top: 10px;
`

export const StyledContent = styled<{}, 'div'>('div')`
  padding: 11px 25px 19px;
  position: relative;
  background: #f9fbfc;
  flex: 1;
`

export const StyledAction = styled<{}, 'button'>('button')`
  display: inline-block;
  background: none;
  padding: 0;
  border: none;
  cursor: pointer;
`

export const StyledActionIcon = styled<{}, 'img'>('img')`
  display: inline-block;
  width: 20px;
  margin-right: 11px;
  vertical-align: text-bottom;
`

export const StyledCopy = styled<StyledProps, 'div'>('div')`
  font-size: 12px;
  color: #838391;
  padding: 19px 15px;
  background: ${p => p.connected ? '#dcdfff' : '#dee2e6'};
  text-align: center;
`

export const StyledCopyImage = styled<{}, 'span'>('span')`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
`

export const StyledIconAction = styled<{}, 'button'>('button')`
  margin-bottom: 17px;
  position: absolute;
  top: 21px;
  right: 21px;
  background: none;
  padding: 0;
  border: none;
  cursor: pointer;
`

export const StyledBalanceConverted = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.17;
  text-align: center;
  color: rgba(255, 255, 255, 0.65);
  margin: 8px 0;
`

export const StyledGrantWrapper = styled<{}, 'div'>('div')`
  margin-top: 13px;
`

export const StyledGrant = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.60);
  text-align: center;
  margin-bottom: 3px;

  b {
    font-weight: 600;
    color: #fff;
    min-width: 81px;
    text-align: right;
    display: inline-block;
  }

  span {
    min-width: 135px;
    text-align: left;
    display: inline-block;
  }
`

export const StyledActionWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  font-size: 12px;
  color: #fff;
  display: flex;
  justify-content: space-around;
  margin: 20px 0 0;
  padding-bottom: 20px;
`

export const StyledBalanceCurrency = styled<{}, 'span'>('span')`
  text-transform: uppercase;
  opacity: 0.66;
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 0.88;
  color: #fff;
`

export const StyledCurve = styled<{}, 'div'>('div')`
  padding: 10px 0;
  position: relative;
  overflow: hidden;
  margin: 0 -21px 0 -19px;
  z-index: 5;

  :before {
    content: "";
    position: absolute;
    bottom: -16px;
    margin-left: -50%;
    height: 240px;
    width: 200%;
    border-radius: 100%;
    border: 20px solid #f9fbfc;
  }
`

export const StyledAlertWrapper = styled<{}, 'div'>('div')`
  display: flex;
  align-items: stretch;
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  z-index: 5;
  width: 100%;
`

export const StyledAlertClose = styled<{}, 'button'>('button')`
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  top: 16px;
  right: 16px;
  cursor: pointer;
`
