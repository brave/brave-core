/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  isInTab?: boolean
}

export const StyledTitleTab = styled<StyleProps, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  padding: 10px ${p => p.isInTab ? 13 : 25}px;
  margin-bottom: -3px;
  border-radius: 6px 6px 0 0;
  backdrop-filter: blur(75px);
  box-shadow: rgba(0, 0, 0, 0.2) 1px 1px 20px 2px;
`
