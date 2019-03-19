/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../theme'
import palette from '../../theme/colors'
import Background from './media/background.svg'
import { BlockedInfoRowStats, BlockedInfoRowText } from './display'
import { StyledWrapper as Toggle } from '../shields/toggle/style'

/**
 * Main wrapper
 */
export const ShieldsPanel = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.heading};
  position: relative;
  background: #FFFFFF;
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
  background-image: url(${Background});
  background-repeat: no-repeat;
  background-position: ${p => p.status === 'enabled' ? '0 -15px' : '0 0'};
  user-select: none;
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
  border-bottom: 1px solid rgba(160, 161, 178, 0.15);

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
  padding: 24px 0 36px;
  border-bottom: 1px solid ${p => p.status === 'enabled' ? 'rgba(160, 161, 178, 0.15)' : 'transparent'};
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
export const BlockedInfoRow = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 1fr auto;
  align-items: center;
  border-bottom: 1px solid rgba(160, 161, 178, 0.15);
  color: ${palette.grey600};
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
  padding: 6px 24px 5px 74px;
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
  padding: 6px 0 5px 20px;
  grid-gap: 2px;
  align-items: center;
  pointer-events: ${p => p.disabled && 'none'};

  & > * {
    opacity: ${p => p.disabled && '0.4'};
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
    outline-color: ${palette.orange400};
  }

  &:hover {
    cursor: pointer;
    color: ${palette.orange400};

    ${BlockedInfoRowStats} {
      color: ${palette.orange400};
    }
    ${BlockedInfoRowText} {
      color: ${palette.orange400};
    }
  }
`

export const BlockedInfoRowDataForSelect = styled(BlockedInfoRowData)`
  grid-template-columns: auto 26px 1fr;
  padding: 12px 0 11px 20px;

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
  background: #FFFFFF;
  width: 100%;
  height: 100%;
  z-index: 2;
  user-select: none;
`

export const BlockedListHeader = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 6px;
  align-items: center;
  padding: 24px 20px 14px;
  border-bottom: 1px solid rgba(160, 161, 178, 0.15);
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
  padding: 6px 24px 5px 20px;
  border-bottom: 1px solid rgba(160, 161, 178, 0.15);

  &:focus {
    outline-width: 2px;
    outline-offset: -3px;
    outline-color: ${palette.orange400};
  }

  &:active {
    outline: none;
  }
`

export const BlockedListStatic = styled<{}, 'ul'>('ul')`
  box-sizing: border-box;
  list-style-type: none;
  height: 307px;
  overflow: auto;
  padding: 0;
  margin: 0 24px;
`

export const BlockedListDynamic = styled(BlockedListStatic)`
  margin: 0;
`

export const BlockedListItemHeader = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  position: sticky;
  top: 0;
  display: grid;
  grid-template-columns: 36px 1fr auto;
  align-items: center;
  padding: 11px 24px 10px 14px;
  margin-bottom: 6px;
  line-height: 1;
  background: rgb(249, 249, 253);
  border-top: 1px solid rgba(160, 161, 178, 0.15);
  border-bottom: 1px solid rgba(160, 161, 178, 0.15);

  &:first-of-type {
    border-top: 0;
  }
`

export const BlockedListItem = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  padding: 9px 0px;
  line-height: 1;
  white-space: nowrap;
  user-select: all;
`

export const BlockedListItemWithOptions = styled<{}, 'li'>('li')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 1fr auto;
  padding: 9px 24px 9px 20px;

  > *:first-child {
    white-space: nowrap;
    overflow-x: hidden;
    text-overflow: ellipsis;
    display: inline-block;
    vertical-align: middle;
    line-height: 1.5;
    font-weight: 500;
    margin: 0px 10px 0px 30px;
    font-size: 12px;
    color: ${palette.grey600};
  }
`

export const BlockedListFooter = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  padding: 12px 0px;
  display: flex;
  justify-content: center;
  border-top: 1px solid rgba(160, 161, 178, 0.15);
`

export const BlockedListFooterWithOptions = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  display: flex;
  justify-content: space-between;
  padding: 12px 24px;
  border-top: 1px solid rgba(160, 161, 178, 0.15);
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
