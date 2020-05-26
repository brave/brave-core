/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledTitleTab = styled<{}, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  padding: 4px 20px 15px 20px;
  margin-bottom: -8px;
  backdrop-filter: blur(23px);
  background: linear-gradient(90deg, rgba(33, 37, 41, 0.1) 0%, rgba(33, 37, 41, 0.22) 100%);
  border-radius: 8px 8px 0 0;

  &:first-child {
    background: linear-gradient(90deg, rgba(33, 37, 41, 1) 0%, rgba(33, 37, 41, 0.4) 100%);
  }

  &:nth-child(1) {
    background: linear-gradient(90deg, rgba(33, 37, 41, 0.8) 0%, rgba(33, 37, 41, 0.32) 100%);
  }

  &:nth-child(2) {
    background: linear-gradient(90deg, rgba(33, 37, 41, 0.6) 0%, rgba(33, 37, 41, 0.24) 100%);
  }

  &:nth-child(3) {
    background: linear-gradient(90deg, rgba(33, 37, 41, 0.4) 0%, rgba(33, 37, 41, 0.16) 100%);
  }
`
