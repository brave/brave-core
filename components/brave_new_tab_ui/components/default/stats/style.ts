/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const StyledStatsItemContainer = styled<{}, 'ul'>('ul')`
  -webkit-font-smoothing: antialiased;
  display: flex;
  flex-wrap: wrap;
  justify-content: var(--ntp-item-justify, start);
  align-items: start;
  font-weight: 400;
  margin: 0;
  padding: 0;
  color: inherit;
  font-size: inherit;
  font-family: inherit;
`

export const StyledStatsItem = styled<{}, 'li'>('li')`
  list-style-type: none;
  font-size: inherit;
  font-family: inherit;
  margin: 10px 16px;

  &:first-child {
    color: #FB542B;
  }
  &:nth-child(2) {
    color: #A0A5EB;
  }
  &:last-child {
    color: #FFFFFF;
    margin-right: 0;
  }
`

export const StyledStatsItemCounter = styled<{}, 'span'>('span')`
  color: inherit;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 46px;
  font-weight: 400;
  width: 7ch;
  text-overflow: ellipsis;
  white-space: nowrap;
  overflow: hidden;
  word-wrap: none;
`

export const StyledStatsItemText = styled<{}, 'span'>('span')`
  font-size: 20px;
  font-family: ${p => p.theme.fontFamily.heading};
  margin-left: 4px;
  display: inline;
  letter-spacing: 0;
`

export const StyledStatsItemDescription = styled<{}, 'div'>('div')`
  font-size: 14px;
  font-weight: 400;
  color: #FFFFFF;
  font-family: ${p => p.theme.fontFamily.heading};
`
