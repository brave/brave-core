/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import Card from '../../../components/layout/card'

export const ShieldsPanel = styled<any, 'main'>('main')`
  box-sizing: border-box;
  background: linear-gradient(to bottom, #131526, #343546);
  border-radius: 6px;
  height: 100%;
  min-height: ${p => p.enabled ? '585px' : '273px'};
  max-height: ${p => p.enabled ? '585px' : '273px'};
  transition: max-height 1.5s ease-in;
`

export const SiteCard = styled(Card)`
  background: #343546;
  margin: 0;
  width: auto;
  text-align: center;
  padding: 16px;
`

export const EnabledText = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  grid-template-columns: auto auto;
  grid-template-rows: 1fr;
  grid-gap: 5px;
  justify-content: center;
  align-items: center;

  &:first-child {
    margin: 0 0 5px;
  }
`

export const DisabledText = styled(EnabledText)`
  grid-template-columns: 1fr 4fr;
  max-width: 90%;
  margin: 10px auto 10px;
`

interface HeaderToggleProps {
  enabled: boolean
}

export const HeaderToggle = styled<HeaderToggleProps, 'div'>('div')`
  box-sizing: border-box;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: ${p => p.enabled ? '0' : '0 0 22px'};
`

interface HeaderProps {
  enabled: boolean
}

export const Header = styled<HeaderProps, 'header'>('header')`
  box-sizing: border-box;
  border-bottom: ${p => p.enabled ? '1px solid rgba(255, 255, 255, 0.15)' : null};
  padding: ${p => p.enabled ? '28px 25px 10px' : '28px 25px 0'};
`
