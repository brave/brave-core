/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  userAuthed?: boolean
}

export const WidgetWrapper = styled<StyleProps, 'div'>('div')`
  color: white;
  padding: 6px 20px 12px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  min-height: ${p => p.userAuthed ? '360px' : 'initial'};
  background: #000;
`

export const Header = styled<{}, 'div'>('div')`
  text-align: left;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: #fff;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const GeminiIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 11px;
  margin-left: -2px;
`

export const StyledTitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`

export const DismissAction = styled<{}, 'span'>('span')`
  display: block;
  cursor: pointer;
  color: #A6A6A6;
  font-size: 14px;
  margin-top: 20px;
  font-weight: bold;
`

export const IntroTitle = styled<{}, 'span'>('span')`
  font-size: 14px;
  margin-top: 10px;
  display: block;
  font-weight: 600;
`

export const Copy = styled<{}, 'p'>('p')`
  font-size: 13px;
  max-width: 240px;
  margin-top: 10px;
  margin-bottom: 20px;
  color: #A6A6A6;
`

export const ActionsWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 5px;
  text-align: center;
`

export const ConnectButton = styled<{}, 'a'>('a')`
  font-size: 14px;
  font-weight: bold;
  border-radius: 20px;
  width: 100%;
  background: #3D3D3D;
  border: 0;
  padding: 10px 60px;
  cursor: pointer;
  color: #fff;
  margin-bottom: 10px;
  text-decoration: none;
  &:focus {
    outline: 0;
  }
`
