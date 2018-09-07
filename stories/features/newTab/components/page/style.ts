/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import theme from '../../../../../src/theme/brave-default'
import palette from '../../../../../src/theme/palette'

export const StyledPage = styled<{}, 'div'>('div')`
  -webkit-font-smoothing: antialiased;
  background: linear-gradient(
    ${theme.color.privateTabBackground},
    ${theme.color.privateTabBackground2}
  );
  min-height: 100%;
  height: initial;
  padding: 40px 60px;
`

export const StyledPageWrapper = styled<{}, 'main'>('main')`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;

  h1 {
    max-width: 800px;
    font-size: 26px;
    font-family: ${theme.fontFamily.heading};
    color: ${palette.white};
    letter-spacing: -0.4px;
    margin: 20px 0 40px;
    align-self: flex-start;
  }
`
