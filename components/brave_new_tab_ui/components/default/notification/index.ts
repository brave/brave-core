/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const SiteRemovalNotification = styled<{}, 'header'>('header')`
  font-family: ${p => p.theme.fontFamily.heading};
  border-radius: 8px;
  box-shadow: 2px 2px 6px rgba(0,0,0,0.3);
  background-color: #fff;
  width: 500px;
  height: 100px;
  align-items: center;
  padding: 30px 60px;
  margin: 80px 0 0 0;
  display: flex;
  justify-content: space-between;
`

export const SiteRemovalText = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  user-select: none;
  font-size: 18px;
`

interface SiteRemovalActionProps {
  iconOnly?: boolean
}

export const SiteRemovalAction = styled<SiteRemovalActionProps, 'a'>('a')`
  font-size: 16px;
  cursor: pointer;
  color: #fb542b;
  width: ${p => p.iconOnly && '16px'};
  line-height: 1;
`
