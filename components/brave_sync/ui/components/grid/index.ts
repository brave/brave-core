/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const SectionBlock = styled<{}, 'section'>('section')`
  margin: 10px 0 40px;
`

interface GridProps {
  columns?: string
  rows?: string
  gap?: string
}

export const Grid = styled<GridProps, 'footer'>('footer')`
  display: grid;
  height: 100%;
  grid-template-columns: ${p => p.columns || '1fr'};
  grid-template-rows: ${p => p.rows || '1fr'};
  grid-template-rows: 1fr;
  grid-gap: ${p => p.gap || '15px'};
  margin: 10px 5px 0;
`

interface FlexColumnProps {
  items?: string
  direction?: string
  content?: string
}

export const FlexColumn = styled<FlexColumnProps, 'div'>('div')`
  display: flex;
  align-items: ${p => p.items};
  flex-direction: ${p => p.direction};
  justify-content: ${p => p.content};
`
