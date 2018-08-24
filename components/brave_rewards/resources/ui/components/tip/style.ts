/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  border-radius: 8px;
  background-image: linear-gradient(148deg, #2825a7, #5465e8), linear-gradient(#696fdc, #696fdc);
  width: 214px;
  overflow: hidden;
  position: relative;
  padding: 16px 0 0;
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 12px;
  line-height: 1.83;
  color: #fff;
  opacity: 0.6;
  padding-left: 23px;
`

export const StyledAllowToggle = styled<{}, 'span'>('span')`
  display: inline-block;
  margin-left: 33px;
  vertical-align: middle;
  padding-top: 2px;
`

export const StyledAllowText = styled<{}, 'span'>('span')`
  opacity: 0.65;
  font-size: 10px;
  line-height: 1.5;
  color: #fff;
`

export const StyledClose = styled<{}, 'button'>('button')`
  position: absolute;
  top: 16px;
  right: 16px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
`

export const StyledTipWrapper = styled<{}, 'div'>('div')`
  display: flex;
  max-width: 160px;
  margin-bottom: 7px;
`
