/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const BaseGrid = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
`

const BaseColumn = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: relative;
  display: flex;
`

export const Footer = styled(BaseGrid.withComponent('footer'))`
  grid-template-columns: 1fr 1fr 1fr;
  grid-template-rows: 1fr;
`

export const FooterLeftColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: flex-start;
`

export const FooterMiddleColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: center;
`

export const FooterRightColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: flex-end;
`

export const Content = styled<{}, 'section'>('section')`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  flex: 1;
  margin-bottom: 50px;
`
