/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledHeader = styled.h1`
  margin: 0 0 18px 0;
  text-align: center;
  font-weight: 500;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 22px;
  color: ${p => p.theme.palette.grey800};
`
