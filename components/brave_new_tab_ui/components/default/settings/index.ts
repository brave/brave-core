/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const SettingsMenu = styled<{}, 'div'>('div')`
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  border-radius: 8px;
  padding: 24px;
  box-shadow: 0px 4px 24px 0px rgba(0, 0, 0, 0.24);
  font-family: ${p => p.theme.fontFamily.body};
`

export const SettingsTitle = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 18px;
  font-weight: bold;
  letter-spacing: 0px;
  margin-bottom: 16px;
`

export const SettingsRow = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 1fr 36px;
  height: 30px;
  width: 320px;
`

export const SettingsText = styled<{}, 'span'>('span')`
  display: flex;
  align-items: center;
  font-size: 14px;
  font-weight: normal;
`

export const SettingsWrapper = styled<{}, 'div'>('div')`
  position: absolute;
  bottom: 118px;
  padding: 0 222px;
  display: flex;
  width: 100%;
  justify-content: flex-end;

  @media screen and (max-width: 904px) {
    padding: 0 192px;
  }
`
