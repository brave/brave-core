/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledExcludedText = styled<{}, 'div'>('div')`
  color: #4B4C5C;
  font-size: 14px;
  font-weight: 300;
  text-align: left;
  letter-spacing: 0;
  margin-top: -20px;
  margin-bottom: 33px;
`

export const StyledRestore = styled<{}, 'a'>('a')`
  color: #696fdc;
  display: inline-block;
  font-size: 13px;
  font-weight: normal;
  letter-spacing: 0;
  cursor: pointer;
  margin-left: 8px;
`
