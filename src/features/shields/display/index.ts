/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import Card from '../../../components/layout/card'

export const ShieldsPanel = styled<any, 'main'>('main')`
  box-sizing: border-box;
  background: linear-gradient(to bottom, #131526, #343546);
  height: 100%;
  position: relative;
`

export const SiteInfoCard = styled(Card)`
  background: #343546;
  margin: 0;
  width: auto;
  text-align: center;
  padding: 16px;
`

interface HeaderProps {
  enabled: boolean
}

export const Header = styled<HeaderProps, 'header'>('header')`
  box-sizing: border-box;
  border-bottom: ${p => p.enabled ? '1px solid rgba(255, 255, 255, 0.15)' : null};
  padding: ${p => p.enabled ? '28px 25px 10px' : '28px 25px 0'};
`

export const ResourcesListScroll = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  overflow: auto;
  margin-top: -3px;
  height: 290px;
`

export const DismissOverlay = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  width: 100%;
  height: 100%;
  background: transparent;
  position: absolute;
  top: 0;
  z-index: 1;
`

export const ClickableEmptySpace = styled<{}, 'div'>('div')`
  width: 25px;
  height: 100%;
`
