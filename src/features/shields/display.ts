/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../theme'
import palette from '../../theme/palette'
import Button, { Props as ButtonProps } from '../../components/buttonsIndicators/button'
import { ComponentType } from 'react'

/**
 * Header
 */
export const MainToggleHeading = styled<{}, 'h1'>('h1')`
  box-sizing: border-box;
  font-size: 16px;
  line-height: 20px;
  font-weight: 500;
  color: ${palette.grey600};
  margin: 0;
`

export const MainToggleText = styled<{}, 'p'>('p')`
  box-sizing: border-box;
  color: ${palette.grey600};
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  line-height: 18px;
  margin: 0;
`

interface ToggleStateTextProps {
  status: 'enabled' | 'disabled'
}

export const ToggleStateText = styled<ToggleStateTextProps, 'span'>('span')`
  box-sizing: border-box;
  color: ${p => p.status === 'enabled' ? palette.orange400 : palette.grey300};
  font-size: inherit;
  text-transform: uppercase;
  font-style: normal;
  font-weight: 600;
`

export const Favicon = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  max-width: 100%;
  width: 24px;
`

export const SiteInfoText = styled<{}, 'p'>('p')`
  box-sizing: border-box;
  font-size: 22px;
  font-weight: 500;
  line-height: 27px;
  color: ${palette.grey800};
  margin: 0;
`

export const TotalBlockedStatsNumber = styled<{}, 'h2'>('h2')`
  box-sizing: border-box;
  font-size: 38px;
  text-transform: uppercase;
  color: ${palette.grey500};
  font-weight: normal;
  margin: 0;
`

export const TotalBlockedStatsText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: ${palette.grey600};
`

/**
 * Interface/Privacy rows
 */
export const BlockedInfoRowStats = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  color: ${palette.grey800};
  font-size: 14px;
  font-weight: 600;
  line-height: 1;
`

export const BlockedInfoRowText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 12px;
  font-weight: 500;
  line-height: 1;
  color: ${palette.grey600};
`

/**
 * Blocked Lists
 */
export const BlockedListSummaryText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 14px;
  font-weight: 500;
  line-height: 1;
  color: ${palette.grey800};
`

export const BlockedListItemHeaderText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-weight: 500;
  color: ${palette.grey600};
  font-size: 12px;
`

export const BlockedListItemHeaderStats = styled<{}, 'span'>('span')`
  text-align: center;
  font-size: 14px;
  color: ${palette.grey800};
  font-weight: 500;
`

/**
 * Buttons that look like links
 */
export const Link = styled<{}, 'button'>('button')`
  box-sizing: border-box;
  -webkit-appearance: none;
  color: ${palette.orange400};
  border: 0;
  padding: 0;
  background: transparent;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  display: inline-block;

  &:hover {
    text-decoration: underline;
  }

  &:focus {
    outline-offset: 6px;
    outline-color: ${palette.orange400};
    outline-width: 2px;
  }

  &:active {
    outline: none;
  }
`

export const LinkAction = styled(Link)`
  box-sizing: border-box;
  color: ${palette.blue400};
  text-decoration: underline;
  font-size: inherit;
  font-weight: 500;

  &:focus {
    outline-offset: initial;
  }

  &:active {
    outline: none;
  }
`

export const DisabledContentText = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  color: ${palette.grey500};
  font-size: 12px;
  font-weight: normal;
  line-height: 18px;
`

/**
 * button
 */
export const ShieldsButton = styled(Button as ComponentType<ButtonProps>)`
  &:focus {
    outline-offset: 2px;
    outline-color: ${palette.orange400};
    outline-width: 2px;
  }

  &:active {
    outline: none;
  }
`
