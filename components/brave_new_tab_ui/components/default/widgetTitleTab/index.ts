/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  isAlone?: boolean
  stackPosition: number
}

const getBackgroundRule = (position: number) => {
  switch (position) {
    case 0:
      return 'linear-gradient(90deg, rgba(33, 37, 41, 1) 0%, rgba(33, 37, 41, 0.4) 100%)'
    case 1:
      return 'linear-gradient(90deg, rgba(33, 37, 41, 0.8) 0%, rgba(33, 37, 41, 0.32) 100%)'
    case 2:
      return 'linear-gradient(90deg, rgba(33, 37, 41, 0.6) 0%, rgba(33, 37, 41, 0.24) 100%)'
    case 3:
      return 'linear-gradient(90deg, rgba(33, 37, 41, 0.4) 0%, rgba(33, 37, 41, 0.16) 100%)'
    default:
      return 'rgba(33, 37, 41, 0.2)'
  }
}

export const StyledTitleTab = styled<StyleProps, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  padding: 4px 20px ${p => p.isAlone ? 5 : 15}px 20px;
  margin-bottom: -8px;
  backdrop-filter: blur(23px);
  border-radius: 8px 8px 0 0;
  background: ${p => getBackgroundRule(p.stackPosition)};
`
