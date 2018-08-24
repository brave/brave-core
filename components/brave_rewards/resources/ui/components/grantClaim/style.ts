/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: flex;
  align-items: stretch;
  align-content: flex-start;
  flex-wrap: nowrap;
  font-family: Poppins, sans-serif;
  border-radius: 6px;
  background-color: #fff;
  overflow: hidden;
  padding-left: 20px;
  margin-bottom: 19px;
  width: 100%;
`

export const StyledIcon = styled<{}, 'div'>('div')`
  flex-basis: 40px;
  border: solid 1px #ffbbaa;
  margin: 12px 0 0;
  border-radius: 50%;
  height: 40px;
  width: 40px;
  flex-grow: 1;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const StyledText = styled<{}, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 70%;
  font-size: 16px;
  font-weight: 500;
  line-height: 1.25;
  color: #fb542b;
  padding: 14px 10px 5px 10px;
`

export const StyledClaim = styled<{}, 'button'>('button')`
  flex-basis: 90px;
  background-color: #fb542b;
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.83;
  letter-spacing: 0.4px;
  text-align: center;
  color: #fff;
  border: none;
  text-transform: uppercase;
  cursor: pointer;
`
