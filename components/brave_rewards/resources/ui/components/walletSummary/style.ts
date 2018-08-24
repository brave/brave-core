/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
  margin-top: 21px;
`

export const StyledSummary = styled<{}, 'div'>('div')`
  font-size: 14px;
  font-weight: 600;
  line-height: 1.57;
  letter-spacing: 0.4px;
  color: #a1a8f2;
  text-transform: uppercase;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 28px;
  font-weight: 300;
  line-height: 0.79;
  letter-spacing: 0.4px;
  color: #4c54d2;
  margin: 4px 0 26px;
  text-transform: uppercase;
`

export const StyledActivity = styled<{}, 'button'>('button')`
  font-size: 12px;
  color: #686978;
  margin-top: 26px;
  text-align: center;
  padding: 0;
  border: none;
  background: none;
  width: 100%;
  cursor: pointer;
`

export const StyledActivityIcon = styled<{}, 'span'>('span')`
  vertical-align: sub;
  margin-right: 11px;
`
