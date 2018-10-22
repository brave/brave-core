/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import palette from '../../../theme/palette'

export const ResourcesLabelGrid = styled('label')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  height: 100%;
  padding: 0 25px 0 20px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.15);
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: ${palette.grey200};
  user-select: none;
  border: 0;
  background-color: rgba(255,255,255,0.15);
  align-items: center;
  display: grid;
  grid-template-columns: 28px 28px 1fr;
  grid-auto-rows: 39px 0;
  grid-gap: 0;
  grid-template-areas: "arrow stat label separator separator separator";

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
    grid-area: separator;
  }
`

export const Resources = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: absolute;
  z-index: 1;
  top: 0;
  left: 0;
  background: linear-gradient(to bottom, #131526, #343546);
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-columns: 1fr;
  grid-template-rows: auto auto 1fr;
  grid-gap: 0;
`

export const ResourcesHeader = styled<{}, 'header'>('header')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  padding: 25px 25px 15px;
  grid-template-columns: 1fr auto;
  grid-template-rows: auto;
  grid-gap: 0;
`

export const ResourcesSiteInfo = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: auto 1fr;
  grid-template-rows: 1fr;
  grid-gap: 5px;
  align-items: center;
`

export const ResourcesList = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  height: 100%;
  padding: 15px 0;
`

export const ResourcesListScroll = styled<{}, 'div'>('div')`
  overflow: auto;
  padding: 0 25px;
  height: 315px;
`

export const ResourcesFooter = styled<{}, 'footer'>('footer')`
  margin: auto;
  padding: 15px 0 0;
  border-top: 1px solid rgba(255,255,255,0.15);
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const ResourcesLabel = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 11px;
`

export const ResourcesListText = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  margin-bottom: 10px;
  line-height: 1;
  color: ${palette.grey500};
  overflow-x: hidden;
`
