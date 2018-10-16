/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const StyledStatsItemContainer = styled<{}, 'ul'>('ul')`
  display: block;
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
  vertical-align: middle;
  list-style-type: none;
  font-size: inherit;
  font-family: inherit;

  &:first-child {
    color: rgb(243, 144, 48);
  }
  &:nth-child(2) {
    color: rgb(254, 82, 29);
  }
  &:nth-child(3) {
    color: rgb(7, 150, 250);
  }
  &:last-child {
    color: rgb(153, 153, 153);
  }
`

export const StyledStatsItemCounter = styled<{}, 'span'>('span')`
  color: inherit;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 44px;
  font-weight: 400;
  line-height: 53px;
  width: 7ch;
  letter-spacing: -0.4px;
  text-overflow: ellipsis;
  white-space: nowrap;
  overflow: hidden;
`

export const StyledStatsItemText = styled<{}, 'span'>('span')`
  color: rgb(153, 153, 153);
  font-size: 20px;
  font-family: ${p => p.theme.fontFamily.heading};
  line-height: 24px;
  margin-left: 3px;
  display: inline;
  letter-spacing: 0;
`

export const StyledStatsItemDescription = styled<{}, 'div'>('div')`
  font-size: 13px;
  font-weight: 400;
  color: rgb(255, 255, 255);
  line-height: 24px;
  margin-left: 3px;
  font-family: ${p => p.theme.fontFamily.heading};
`
