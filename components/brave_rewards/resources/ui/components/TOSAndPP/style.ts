/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled('div')<{}>`
  font-size: 14px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 18px;
`

export const StyledLink = styled('a')<{}>`
  text-decoration: none;
  cursor: pointer;
  color: ${p => p.theme.palette.blurple500};
  font-weight: 600;
`
