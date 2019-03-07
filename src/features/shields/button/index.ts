/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const EmptyButton = styled<{}, 'button'>('button')`
  box-sizing: border-box;
  display: flex;
  background: transparent;
  border: 0;
  padding: 0;
  margin: 0;
  cursor: pointer;

  &:disabled {
    opacity: 0.3;
    pointer-events: none;
  }
`

export const CloseButton = styled<{}, 'button'>('button')`
  box-sizing: border-box;
  background: transparent;
  display: flex;
  border: 0;
  padding: 0;
  margin: 0;
  cursor: pointer;
`
