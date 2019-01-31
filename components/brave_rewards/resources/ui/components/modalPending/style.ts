/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 20px;
  font-weight: 600;
  line-height: 2;
  color: #4b4c5c;
  margin-bottom: 20px;
  display: flex;
`
export const StyledRemoveAll = styled<{}, 'button'>('button')`
  display: inline-block;
  border: none;
  background: none;
  color: #696fdc;
  font-size: 13px;
  font-weight: normal;
  cursor: pointer;
  margin: 6px 5px 0 0;
`
