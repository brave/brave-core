/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'

export const StyledStatsItemContainer = styled('ul')<{}>`
  -webkit-font-smoothing: antialiased;
  display: inline-flex;
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

export const StyledStatsItem = styled('li')<{overriddenTextColor?: string}>`
  list-style-type: none;
  font-size: inherit;
  font-family: inherit;
  margin: 10px 16px;
  &:last-child { margin-right: 0; }

  ${p => p.overriddenTextColor
    ? css`color: ${p.overriddenTextColor};`
    : css`&:first-child { color: var(--interactive2); }
          &:nth-child(2) { color: var(--interactive9); }
          &:last-child { color: #FFFFFF; }`}
`

export const StyledStatsItemCounter = styled('span')<{}>`
  color: inherit;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 40px;
  font-weight: 400;
  width: 7ch;
  text-overflow: ellipsis;
  white-space: nowrap;
  overflow: hidden;
  word-wrap: none;
`

export const StyledStatsItemText = styled('span')<{}>`
  font-size: 24px;
  font-family: ${p => p.theme.fontFamily.heading};
  margin-left: 4px;
  display: inline;
  letter-spacing: 0;
`

export const StyledStatsItemDescription = styled('div')<{overriddenTextColor?: string}>`
  font-size: 16px;
  font-weight: 500;
  ${p => !p.overriddenTextColor && css`color: #FFFFFF;`}
  margin-top: 8px;
  font-family: ${p => p.theme.fontFamily.heading};
`
