/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
  position: relative;
  display: flex;
  width: 100%;
  border-radius: 6px;
  background-color: #dee2e6;
  border: 1px solid #dbdfe3;
  justify-content: space-between;
  align-items: flex-start;
  align-content: flex-start;
  flex-wrap: wrap;
  padding: 22px 34px 18px;
  margin-bottom: 25px;
`

export const StyledLeft = styled<{}, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  display: flex;
  align-items: center;
`

export const StyledRight = styled<{}, 'div'>('div')`
  padding: 10px 0 0;
  display: flex;
  height: 66px;
  align-items: center;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 28px;
  font-weight: 600;
  letter-spacing: 0.2px;
  color: #4b4c5c;
  margin: 4px 0 0 11px;
`

export const StyledTM = styled<{}, 'span'>('span')`
  font-size: 10px;
  font-weight: 300;
  letter-spacing: 0.2px;
  text-align: center;
  color: #222326;
  vertical-align: text-top;
`

export const StyleTitle = styled<{}, 'div'>('div')`
  font-size: 22px;
  line-height: 1.27;
  color: #4b4c5c;
  margin-top: 40px;
`

export const StyleText = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 16px;
  font-weight: 300;
  line-height: 1.75;
  color: #838391;
`

export const StyledContent = styled<{}, 'div'>('div')`
  flex-basis: 100%;
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
  width: 66px;
  height: 66px;
`
