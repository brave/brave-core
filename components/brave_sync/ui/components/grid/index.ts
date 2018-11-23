/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const SectionBlock = styled<{}, 'section'>('section')`
  margin: 10px 0 40px;
`

export const EnabledContentButtonGrid = styled<{}, 'footer'>('footer')`
  display: grid;
  grid-template-columns: 1fr;
  grid-template-rows: 1fr;
  grid-gap: 5px;
  margin: 10px 5px 0;
`

export const SettingsToggleGrid = styled<{}, 'footer'>('footer')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-template-rows: 1fr 1fr 1fr;
  grid-gap: 5px;
  align-items: center;
  margin: 15px 0 0;
`

export const DisabledContentButtonGrid = styled<{}, 'footer'>('footer')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-template-rows: 1fr;
  grid-gap: 10px;
  margin: 20px 0;
`

export const TableGrid = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: auto auto;
  grid-gap: 20px;
`

export const TableButtonGrid = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-rows: 40px 40px;
  grid-gap: 15px;
`
