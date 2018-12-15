/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const StyledStatsItemContainer = styled<{}, 'ul'>('ul')`
  display: flex;
  flex-wrap: wrap;
  font-weight: 400;
  margin: 0;
  padding: 0;
  color: inherit;
  font-size: inherit;
  font-family: inherit;

  & > li {
    display: inline-block;
    margin-right: 40px;
    margin-bottom: 20px;
  }
`

export const StyledStatsItem = styled<{}, 'li'>('li')`
  list-style-type: none;
  font-size: inherit;
  font-family: inherit;

  &:first-child {
    color: #FB542B;
  }
  &:nth-child(2) {
    color: #B02FFB;
  }
  &:nth-child(3) {
    color: #4C54D2;
  }
  &:last-child {
    color: #FFF;
  }
`

export const StyledStatsItemCounter = styled<{}, 'span'>('span')`
  color: inherit;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 46px;
  font-weight: 400;
  text-overflow: ellipsis;
  white-space: nowrap;
  overflow: hidden;
`

export const StyledStatsItemText = styled<{}, 'span'>('span')`
  color: #FFF;
  font-size: 20px;
  margin-left: 4px;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledStatsItemDescription = styled<{}, 'div'>('div')`
  font-size: 14px;
  font-weight: 400;
  color: #FFF;
  font-family: ${p => p.theme.fontFamily.heading};
`
