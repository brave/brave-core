/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'

interface Props {
  textDirection: string
}

export const SettingsMenu = styled<Props, 'div'>('div')`
  bottom: 48px;
  width: auto;
  position: absolute;
  ${p => p.textDirection && (p.textDirection === 'rtl') ? `left: 12px` : `right: 12px`}
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  border-radius: 8px;
  padding: 24px 24px 17px 24px;
  box-shadow: 0px 4px 24px 0px rgba(0, 0, 0, 0.24);
  font-family: ${p => p.theme.fontFamily.body};
`

export const SettingsTitle = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 18px;
  font-weight: bold;
  letter-spacing: 0px;
  margin-bottom: 17px;
`

interface SettingsRowProps {
  isChildSetting?: boolean
}

export const SettingsRow = styled<SettingsRowProps, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 1fr 36px;
  margin-top: 10px;
  height: 30px;
  width: 320px;
  ${p => p.isChildSetting && css`
    padding-left: 15px;
  `}
`

export const SettingsText = styled<{}, 'span'>('span')`
  display: flex;
  align-items: center;
  font-size: 14px;
  font-weight: normal;
`

export const SettingsWrapper = styled<{}, 'div'>('div')`
  position: relative;
  display: flex;
  justify-content: flex-end;
`
