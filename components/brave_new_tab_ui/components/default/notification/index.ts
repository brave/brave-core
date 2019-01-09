/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

export const SiteRemovalNotification = styled<{}, 'header'>('header')`
  box-sizing: border-box;
  display: grid;
  grid-gap: 16px;
  grid-template-columns: 1fr auto auto auto;
  font-family: ${p => p.theme.fontFamily.heading};
  border-radius: 8px;
  box-shadow: 2px 2px 6px rgba(0, 0, 0, 0.3);
  background-color: #fff;
  width: 500px;
  height: 100px;
  align-items: center;
  padding: 30px 60px;
  margin: 80px auto 0;
`

export const SiteRemovalText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  user-select: none;
`

interface SiteRemovalActionProps {
  iconOnly?: boolean
}

export const SiteRemovalAction = styled<SiteRemovalActionProps, 'a'>('a')`
  box-sizing: border-box;
  font-size: 14px;
  text-decoration: none;
  cursor: pointer;
  color: #fb542b;
  width: ${p => p.iconOnly && '14px'};
  line-height: 1;
`
