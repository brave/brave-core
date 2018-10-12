/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
  display: flex;
  width: 100%;
  background-color: #fff;
  justify-content: space-between;
  align-items: flex-start;
  align-content: flex-start;
  flex-wrap: wrap;
  position: fixed;
  top: 0;
  left: 0;
  height: 61px;
  z-index: 2;
  box-shadow: 0 0 2px 0 rgba(99,105,110,0.55);
`

export const StyledLeft = styled<{}, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  display: flex;
  align-items: center;
  padding: 0px 13px;
`

export const StyledRight = styled<{}, 'div'>('div')`
  padding: 0px 15px;
  display: flex;
  height: 66px;
  align-items: center;
  margin-top: -3px;

  @media (max-width: 355px) {
    width: 55px;
    position: relative;
    right: 45px;
  }
`

export const StyledTitle = styled<{}, 'div'>('div')`
  color: #4B4C5C;
  font-size: 22px;
  font-weight: 500;
  letter-spacing: 0.12px;
  line-height: 44px;
  margin-bottom: -23px;
`

export const StyledTM = styled<{}, 'span'>('span')`
  font-size: 8px;
  font-weight: 300;
  letter-spacing: 0.2px;
  text-align: center;
  color: #222326;
  position: relative;
  top: -13px;
  vertical-align: text-top;
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
  width: 30px;
  height: 30px;
  margin-top: 13px;
  margin-right: 5px;
`
