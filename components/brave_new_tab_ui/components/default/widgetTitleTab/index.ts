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
  backdrop-filter: blur(25px);
  background: linear-gradient(90deg, rgba(33,37,41,0.4) 0%, rgba(33,37,41,0.16) 100%);
  border-radius: 8px 8px 0 0;
  box-shadow: rgba(0, 0, 0, 0.2) 1px 1px 20px 2px;
`
