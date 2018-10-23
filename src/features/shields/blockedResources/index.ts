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

export const ResourcesLabelGridStatic = styled('label')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  height: 100%;
  padding: 0 25px 0 20px;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: ${palette.grey200};
  user-select: none;
  border: 0;
  align-items: center;
  display: grid;
  grid-template-columns: 28px 28px 1fr;
  grid-auto-rows: 39px 0;
  grid-gap: 0;
  grid-template-areas: "arrow stat label separator separator separator";
  background-color: rgba(255, 255, 255, 0.15);

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

export const ResourcesLabelGridDynamic = styled('label')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  height: 100%;
  padding: 0 25px 0 20px;
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
  grid-template-columns: 28px 1fr;
  grid-auto-rows: 39px 0;
  grid-gap: 0;
  background-color: rgba(255, 255, 255, 0.15);
`

export const ResourcesBlockedLabelGrid = styled(ResourcesLabelGridStatic)`
  display: grid;
  height: 100%;
  grid-template-columns: auto 1fr;
  grid-template-rows: auto;
  align-items: center;
  height: auto;
  grid-template-areas: unset;
  padding: 10px 25px 5px 20px;
  background-color: transparent;
`

interface ResourcesLabelScriptsProps {
  accent?: 'blocked' | 'allowed'
}

export const ResourcesLabelScripts = styled<ResourcesLabelScriptsProps, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 5px;
  color: ${p => p.accent === 'blocked' ? palette.red500 : palette.green500};
`

export const ResourcesLabelScriptsAllowed = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 5px;
  margin-left: -15px;
  font-size: 13px;
  color: ${palette.green500};
  display: none;
`

export const ResourcesLabelScriptsBlocked = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 5px;
  margin-left: -15px;
  font-size: 13px;
  color: ${palette.red500};
  display: none;
`

export const ResourcesLink = styled<{}, 'a'>('a')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  display: flex;
  justify-content: space-between;
  align-items: center;
  color: ${palette.blue200};
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: normal;
  text-decoration: underline;
  cursor: pointer;

  &:hover {
    color: ${palette.white};
    * {
      fill: ${palette.white};
    }
  }
`

export const ResourcesLinkUndo = styled(ResourcesLink)`
  display: none;
`

export const ResourcesBlockedLabelGrid2 = styled(ResourcesLabelGridStatic)`
  display: grid;
  height: 100%;
  grid-template-columns: auto 1fr;
  grid-template-rows: auto;
  align-items: center;
  height: auto;
  grid-template-areas: unset;
  padding: 5px 25px 5px 20px;
  grid-gap: 10px;
  background-color: transparent;

  &:hover {
    background-color: rgba(255, 255, 255, 0.15);

    ${ResourcesLink} {
      display: none;
    }

    ${ResourcesLabelScriptsAllowed} {
      display: block;
    }

    ${ResourcesLabelScriptsBlocked} {
      display: block;
    }

    ${ResourcesLinkUndo} {
      display: block;
    }
  }
`

export const ResourcesBlockedLabel2 = styled<{}, 'span'>('span')`
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
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
  padding: 0;
`

export const ResourcesListScroll = styled<{}, 'div'>('div')`
  overflow: auto;
  height: 330px;
`

export const ResourcesFooter = styled<{}, 'footer'>('footer')`
  margin: auto;
  padding: 15px 0;
  border-top: 1px solid rgba(255,255,255,0.15);
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const ResourcesFooterGrid = styled<{}, 'footer'>('footer')`
  border-top: 1px solid rgba(255,255,255,0.15);
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr;
  padding: 0 20px;
  grid-gap: 0;
`

export const ResourcesFooterGridColumn1 = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  height: 65px;
`

export const ResourcesFooterGridColumn2 = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  height: 65px;
`

export const ResourcesLabel = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 11px;
`

export const ResourcesLabelStatic = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0 11px;
`

export const ResourcesLabelDynamic = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 0;
  font-size: 14px;
`

export const ResourcesListText = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  margin-bottom: 10px;
  line-height: 1;
  color: ${palette.grey500};
  overflow-x: hidden;
  padding: 0 25px;

  &:first-of-type {
    margin-top: 10px;
  } 
`
