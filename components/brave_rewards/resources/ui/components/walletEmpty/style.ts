/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  padding-top: 32px;
  text-align: center;
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 22px;
  font-weight: 300;
  line-height: 1.05;
  color: #999ea2;
  margin-top: 32px;
`

export const StyledContent = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.57;
  color: #999ea2;
  margin: 36px auto 31px;
  text-align: left;
  max-width: 250px;
  width: 100%;

  b {
    font-weight: 600;
    color: #686978;
  }
`
