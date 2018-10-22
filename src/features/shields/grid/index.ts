/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import palette from '../../../theme/palette'

export const Grid = styled<{}, 'section'>('section')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: 28px 28px 1fr auto;
  grid-auto-rows: 46px 0;
  grid-gap: 0;
  padding: 0 25px 0 20px;
  align-items: center;
  border-bottom: 1px solid rgba(255, 255, 255, 0.15);
  grid-template-areas: "arrow stat label toggle separator separator separator";

	> *:nth-child(4n+1) {
		grid-area: arrow;
  }

	> *:nth-child(4n+2) {
		grid-area: stat;
	}

	> *:nth-child(4n+3) {
		grid-area: label;
	}

  > *:nth-child(4n+4) {
		grid-area: toggle;
	}

  > *:nth-child(4n+5) {
    grid-area: separator;
  }
`

export const GridLabel = styled(Grid)`
  -webkit-font-smoothing: antialiased;
  grid-auto-rows: 39px 0;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: ${palette.grey200};
  user-select: none;
`
