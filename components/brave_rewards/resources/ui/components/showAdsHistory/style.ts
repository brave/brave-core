/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  padding-top: 10px;
  text-align: right;
`

export const StyledLink = styled<{}, 'a'>('a')`
  cursor: pointer;
  display: inline-block;
  color: #696FDC;
  font-size: 13px;
  font-weight: normal;
  letter-spacing: 0;
`

export const StyledDisabledLink = styled<{}, 'span'>('span')`
  display: inline-block;
  color: #CED0DB;
  cursor: pointer;
  font-size: 13px;
  font-weight: normal;
  letter-spacing: 0;
`
