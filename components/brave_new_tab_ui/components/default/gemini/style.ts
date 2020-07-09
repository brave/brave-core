/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  userAuthed?: boolean
  isActive?: boolean
  isLast?: boolean
  hideOverflow?: boolean
  isDeposit?: boolean
  clickable?: boolean
  isDetail?: boolean
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
  margin-right: 7px;
  margin-left: 2px;
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

export const NavigationBar = styled<{}, 'div'>('div')`
  height: 30px;
  margin-top: 15px;
  &:first-child {
    margin-right: 3px;
  }
`

export const NavigationItem = styled<StyleProps, 'button'>('button')`
  float: left;
  width: 25%;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  text-align: center;
  background: inherit;
  border: none;
  color: ${p => p.isActive ? '#EEEEEE' : '#8F8F8F'};
  margin-right: ${p => p.isLast ? 12 : 0}px
  margin-left: 0px;
  &:focus {
    outline: 0;
  }
`

export const SelectedView = styled<StyleProps, 'div'>('div')`
  border: 1px solid rgb(70, 70, 70);
  overflow-y: ${p => p.hideOverflow ? 'hidden' : 'scroll'};
  height: 260px;
  width: 240px;
  margin-left: 4px;
`

export const ListItem = styled<{}, 'div'>('div')`
  border-bottom: 1px solid rgb(70, 70, 70);
  padding: 6px 5px;
  overflow-y: auto;
  overflow-x: hidden;
`

export const ListIcon = styled<{}, 'div'>('div')`
  margin-left: 5px;
  width: 28px;
  margin-top: 6px;
  float: left;
  margin-right: 10px;
`

export const ListImg = styled<{}, 'img'>('img')`
  width: 20px;
  margin-top: -6px;
`

export const ListLabel = styled<StyleProps, 'div'>('div')`
  color: #fff;
  cursor: ${p => p.clickable ? 'pointer' : 'initial'};
  margin-top: 10px;
  font-weight: 600;
`

export const AssetIconWrapper = styled<StyleProps, 'div'>('div')`
  height: 25px;
  width: 25px;
  border-radius: 100px;
`

export const AssetIcon = styled<StyleProps, 'span'>('span')`
  margin-top: 6px;
  margin-left: 6px;
`

export const SearchInput = styled<{}, 'input'>('input')`
  border: none;
  color: #fff;
  background: inherit;
  font-size: 15px;
  &:focus {
    outline: 0;
  }
`
