/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../theme'
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
  color: ${p => p.theme.color.text};
  margin: 0;
`

export const MainToggleText = styled<{}, 'p'>('p')`
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  margin: 0;
  line-height: 1.2;
`

interface ToggleStateTextProps {
  status: 'enabled' | 'disabled'
}

export const ToggleStateText = styled<ToggleStateTextProps, 'span'>('span')`
  box-sizing: border-box;
  color: ${p => p.status === 'enabled' ? p => p.theme.color.brandBrave : p.theme.color.text};
  font-size: inherit;
  text-transform: uppercase;
  font-style: normal;
  font-weight: 500;
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
  color: ${p => p.theme.color.text};
  margin: 0;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 300px;
`

export const TotalBlockedStatsNumber = styled<{}, 'h2'>('h2')`
  box-sizing: border-box;
  font-size: 38px;
  text-transform: uppercase;
  color: ${p => p.theme.color.text};
  font-weight: normal;
  text-align: right;
  margin: 0;
`

export const TotalBlockedStatsText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  color: ${p => p.theme.color.text};
`

/**
 * Interface/Privacy rows
 */
export const BlockedInfoRowStats = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
  font-size: 14px;
  font-weight: 600;
  line-height: 1;
`

export const BlockedInfoRowText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 12px;
  font-weight: 500;
  line-height: 1;
  color: ${p => p.theme.color.text};
`

/**
 * Blocked Lists
 */
export const BlockedListSummaryText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 14px;
  font-weight: 500;
  line-height: 1;
  color: ${p => p.theme.color.text};
`

export const BlockedListItemHeaderText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-weight: 600;
  color: ${p => p.theme.color.text};
  font-size: 14px;
`

export const BlockedListItemHeaderStats = styled<{}, 'span'>('span')`
  text-align: center;
  font-size: 14px;
  color: ${p => p.theme.color.text};
  font-weight: 600;
`

/**
 * Buttons that look like links
 */
export const Link = styled<{}, 'button'>('button')`
  box-sizing: border-box;
  -webkit-appearance: none;
  color: ${p => p.theme.color.brandBrave};
  border: 0;
  padding: 0;
  background: transparent;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  display: inline-block;

  &:hover {
    text-decoration: underline;
  }

  &:focus {
    outline-offset: 6px;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 2px;
  }

  &:active {
    outline: none;
  }
`
interface LinkActionProps {
  size?: 'small'
}

export const LinkAction = styled<LinkActionProps, any>(Link)`
  box-sizing: border-box;
  color: ${p => p.theme.color.brandBrave};
  font-size: ${p => p.size === 'small' && '12px' || 'inherit'};
  line-height: ${p => p.size === 'small' && '1'};
  font-weight: 500;

  &:focus {
    outline-offset: initial;
  }

  &:active {
    outline: none;
  }

  &:disabled {
    color: ${p => p.theme.color.disabledResourceBlocked};
    font-style: italic;
    pointer-events: none;
    cursor: default;

    &:hover {
      text-decoration: none;
    }
  }
`

export const DisabledContentText = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  color: ${p => p.theme.color.text};
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
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 2px;
  }

  &:active {
    outline: none;
  }
`
