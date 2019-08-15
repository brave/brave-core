/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const StyledStatsItemContainer = styled<{}, 'ul'>('ul')`
  -webkit-font-smoothing: antialiased;
  display: grid;
  grid-template-columns: repeat(3, fit-content(100%));
  grid-gap: 25px 50px;
  font-weight: 400;
  margin: 0;
  padding: 0;
  color: inherit;
  font-size: inherit;
  font-family: inherit;

  @media screen and (max-width: 700px) {
    grid-template-columns: repeat(2, fit-content(100%));
  }


  @media screen and (max-width: 390px) {
    grid-template-columns: repeat(1, fit-content(100%));
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
    color: #A0A5EB;
  }
  &:last-child {
    color: #FFFFFF;
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
