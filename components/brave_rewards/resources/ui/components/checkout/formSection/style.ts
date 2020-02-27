/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const Box = styled.div`
  border-radius: 8px;
  background: ${p => p.theme.color.panelBackgroundSecondary};
  box-shadow: 0px 0px 6px 0 rgba(12, 23, 33, 0.20);
  padding: 16px;
  margin: 29px 0 14px;
  position: relative;
  top: 0;
  left: 0;
`

export const Header = styled.h2`
  font-weight: 500;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  color: ${p => p.theme.palette.blurple600};
  margin: 0 0 5px 0;
  display: flex;
  justify-content: space-between;
`
