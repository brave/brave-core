/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Card } from '../../../index'

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

export const SelectGrid = styled(BaseGrid)`
  display: grid;
  height: 100%;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr;
  padding: 0 30px;
  display: grid;
  height: 100%;
  grid-template-columns: 2fr 1fr;
  grid-template-rows: 1fr;
  grid-gap: 30px;
  align-items: center;
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

export const Panel = styled(Card)`
  background-color: rgba(255,255,255,0.95);
  border-radius: 20px;
  box-shadow: 0 6px 12px 0 rgba(39, 46, 64, 0.2);
  max-width: 600px;
  min-height: 580px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding: 50px 60px
`
