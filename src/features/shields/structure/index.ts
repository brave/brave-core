/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import { BlockedInfoRowStats, BlockedInfoRowText } from '../display'
import { StyledWrapper as Toggle } from '../toggle/style'
import dash from './dash.svg'

/**
 * Main wrapper
 */
export const ShieldsPanel = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.heading};
  position: relative;
  background: ${p => p.theme.color.panelBackground};
  height: 100%;
  display: grid;
  grid-template-columns: 1fr;
  grid-template-rows: auto auto auto auto;
`

interface ShieldsHeaderProps {
  status: 'enabled' | 'disabled'
}

export const ShieldsHeader = styled<ShieldsHeaderProps, 'header'>('header')`
  box-sizing: border-box;
  user-select: none;
  background: ${p => p.theme.color.panelBackgroundSecondary};
  border-radius: 0 0 200px 50%/25px;
  margin: 0 0 8px;
`

/**
 * Header
 */
interface MainToggleProps {
  status: 'enabled' | 'disabled'
}

export const MainToggle = styled<MainToggleProps, 'section'>('section')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 3fr 1fr;
  padding: 16px 24px 16px;
  align-items: ${p => p.status === 'enabled' ? null : 'center'};
  border-bottom: 1px solid ${p => p.theme.color.separatorLine};

  > ${Toggle} {
    display: flex;
    justify-content: flex-end;
    width: fit-content;
    height: 100%;
    margin-left: 54px;
    align-items: center;
  }
`

interface SiteOverviewProps {
  status: 'enabled' | 'disabled'
}

export const SiteOverview = styled<SiteOverviewProps, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  align-items: center;
  justify-content: center;
  padding: 32px 0;
`

export const TotalBlockedStats = styled<{}, 'section'>('section')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 80px 140px;
  align-items: center;
  margin: 0px auto 5px;
  grid-gap: 10px;
`

export const SiteInfo = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: auto 1fr;
  align-items: center;
  margin: 0px auto 5px;
  gap: 8px;
`

/**
 * Interface/privacy Rows
 */

interface BlockedInfoRowProps {
  extraColumn?: boolean
}

export const BlockedInfoRow = styled<BlockedInfoRowProps, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: ${p => p.extraColumn ? '1fr auto auto' : '1fr auto'};
  grid-gap: ${p => p.extraColumn ? '4px' : '0'};
  align-items: center;
  color: ${p => p.theme.color.text};
  user-select: none;

  &:active {
    outline: none;
  }

  ${Toggle} {
    position: relative;
    z-index: 1;
    display: flex;
    width: fit-content;
    padding: 0 24px;
    height: fit-content;
    height: 100%;
    align-items: center;
  }
`

export const BlockedInfoRowSingle = styled(BlockedInfoRow)`
  padding: 8px 24px 8px 74px;
  cursor: default;
  grid-template-columns: 1fr;
`

export const BlockedInfoRowForSelect = styled(BlockedInfoRow)`
  grid-template-columns: auto 1fr;
  padding-right: 24px;
`

interface BlockedInfoRowDataProps {
  disabled: boolean
}

export const BlockedInfoRowData = styled<BlockedInfoRowDataProps, 'div'>('div')`
  display: grid;
  grid-template-columns: auto 36px 1fr;
  padding: 8px 0 8px 20px;
  grid-gap: 2px;
  align-items: center;
  pointer-events: ${p => p.disabled && 'none'};

  & > * {
    opacity: ${p => p.disabled && '0.6'};
  }

  & > ${BlockedInfoRowText} {
    opacity: 1;
  }

  &:active {
    outline: none;
  }

  &:focus {
    outline-width: 2px;
    outline-offset: -3px;
    outline-color: ${p => p.theme.color.brandBrave};
  }

  &:hover {
    cursor: pointer;
    color: ${p => p.theme.color.brandBrave};

    ${BlockedInfoRowStats} {
      color: ${p => p.theme.color.brandBrave};
    }
    ${BlockedInfoRowText} {
      color: ${p => p.theme.color.brandBrave};
    }
  }

  &:hover svg {
    color: ${p => p.theme.color.brandBrave};
  }
`

export const BlockedInfoRowDataForSelect = styled(BlockedInfoRowData)`
  grid-template-columns: auto 26px 1fr;
  padding: 8px 0 8px 20px;

  &:hover {
    cursor: pointer;
  }
`

/**
 * Footer
 */
export const MainFooter = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  padding: 24px;
`

/**
 * Blocked Lists
 */
export const BlockedListContent = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: absolute;
  top: 0;
  left: 0;
  background: ${p => p.theme.color.panelBackground};
  width: 100%;
  height: 100%;
  z-index: 2;
  cursor: default;
`

export const BlockedListHeader = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 6px;
  align-items: center;
  padding: 24px 20px 14px;
`

interface BlockedListSummaryProps {
  stats?: boolean
}

export const BlockedListSummary = styled<BlockedListSummaryProps, 'summary'>('summary')`
  &::-webkit-details-marker {
    display: none;
  }

  cursor: pointer;
  display: grid;
  grid-template-columns: ${p => p.stats === false ? '30px 1fr' : '28px 28px 1fr'};
  align-items: center;
  padding: 8px 24px 8px 20px;

  &:focus {
    outline-width: 2px;
    outline-offset: -3px;
    outline-color: ${p => p.theme.color.brandBrave};
  }

  &:active {
    outline: none;
  }
`

export const BlockedListStatic = styled<{}, 'ul'>('ul')`
  box-sizing: border-box;
  list-style-type: none;
  height: 330px;
  overflow: auto;
  padding: 0 0 0 24px;
  margin: 0;
  font-size: 12px;
  background: ${p => p.theme.color.panelBackgroundSecondary};
`

export const BlockedListDynamic = styled(BlockedListStatic)`
  margin: 0;
  padding: 0;
`

export const BlockedListItemHeader = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  position: sticky;
  top: 0;
  z-index: 1;
  display: grid;
  grid-template-columns: 36px 1fr auto;
  align-items: center;
  padding: 12px 24px 12px 14px;
  line-height: 1;
  background: ${p => p.theme.color.panelBackground};

  &:first-of-type {
    border-top: 0;
  }
`

export const BlockedListItem = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  padding: 8px 0px;
  line-height: 1;
  margin: auto 0;
  white-space: nowrap;
  user-select: all;
  color: ${p => p.theme.color.text}
`

export const BlockedListItemWithOptions = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  position: relative;
  display: grid;
  grid-template-columns: 1fr auto;
  padding: 8px 24px 8px 20px;
  background: ${p => p.theme.color.panelBackgroundSecondary};

  > *:first-child {
    white-space: nowrap;
    overflow-x: hidden;
    text-overflow: ellipsis;
    display: inline-block;
    vertical-align: middle;
    line-height: 1.5;
    font-weight: 600;
    margin: 0px 10px 0px 30px;
    font-size: 12px;
    color: ${p => p.theme.color.text};
  }
`

export const BlockedListItemDetails = styled<{}, 'details'>('details')`
  position: relative;

  &[open] summary:after {
    content: "-";
    color: ${p => p.theme.color.text};
  }

  /* grouped scripts row (nested dash) */
  &[open] ${BlockedListItemWithOptions}:before {
    content: " ";
    background: url(${dash});
    background-repeat: no-repeat;
    position: absolute;
    z-index: 0;
    top: -2px;
    left: 26px;
    width: 100%;
    height: 100%;
  }
  ${BlockedListItemWithOptions} {
    padding: 4px 24px 4px 20px;
  }
`

export const BlockedListItemSummary = styled(BlockedListItemWithOptions.withComponent('summary'))`
  position: relative;

  &::-webkit-details-marker {
    display: none;
  }

  &:after {
    position: absolute;
    content: "+";
    color: ${p => p.theme.color.text};
    top: 7px;
    left: 26px;
    font-size: 20px;
    padding: 0;
  }

  &:focus {
    outline-width: 2px;
    outline-offset: -3px;
    outline-color: ${p => p.theme.color.brandBrave};
  }
`

export const BlockedListFooter = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  padding: 8px 0px;
  display: flex;
  justify-content: center;
`

export const BlockedListFooterWithOptions = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  display: flex;
  justify-content: space-between;
  padding: 8px 24px;
`

/**
 * Disabled content
 */
export const DisabledContentView = styled<{}, 'section'>('section')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 2fr 5fr;
  grid-gap: 4px;
  align-items: center;
  max-width: 80%;
  margin: 5px auto 8px;
`
