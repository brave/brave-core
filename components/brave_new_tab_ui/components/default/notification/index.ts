/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'

export const SiteRemovalNotification = styled('header')<{}>`
  font: ${font.default.regular};
  border-radius: 8px;
  box-shadow: 2px 2px 6px rgba(0,0,0,0.3);
  background-color: ${color.container.background};
  color: ${color.text.primary};
  align-items: center;
  padding: 30px;
  display: grid;
  grid-auto-flow: column;
  grid-auto-columns: auto;
  grid-gap: 15px;
`

export const SiteRemovalText = styled('span')<{}>`
  box-sizing: border-box;
  user-select: none;
`

interface SiteRemovalActionProps {
  iconOnly?: boolean
}

export const SiteRemovalAction = styled('a')<SiteRemovalActionProps>`
  cursor: pointer;
  color: ${color.icon.interactive};
  width: ${p => p.iconOnly && '16px'};
  text-decoration: underline;
`
