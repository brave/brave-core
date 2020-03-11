/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  hide?: boolean
  itemsShowing?: boolean
  isSmall?: boolean
  isInTab?: boolean
  isLast?: boolean
  disabled?: boolean
  isFiat?: boolean
  isActive?: boolean
}

export const WidgetWrapper = styled<{}, 'div'>('div')`
  color: white;
  padding: 10px 15px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  background-image: linear-gradient(140deg, #1F2327 0%, #000000 100%);
`

export const Header = styled<StyleProps, 'div'>('div')`
  margin-top: ${p => p.isInTab ? 0 : 10}px;
  text-align: left;
`

export const Content = styled<{}, 'div'>('div')`
  margin: 10px 0;
  min-width: 255px;
`

export const Title = styled<{}, 'span'>('span')`
  display: block;
  font-size: 13px;
  font-weight: bold;
`

export const Copy = styled<{}, 'p'>('p')`
  font-size: 15px;
  max-width: 240px;
  margin-top: 20px;
  margin-bottom: 11px;
`

export const BuyPromptWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 20px;
`

export const FiatInputWrapper = styled<{}, 'div'>('div')`
  height: 30px;
  border: 1px solid rgb(70, 70, 70);
  margin-bottom: 10px;
`

export const FiatDropdown = styled<StyleProps, 'div'>('div')`
  float: right;
  width: 25%;
  padding: 7px 3px 0px 7px;
  cursor: ${p => p.disabled ? 'auto' : 'pointer'};
`

export const CaratDropdown = styled<StyleProps, 'div'>('div')`
  width: 14px;
  height: 14px;
  float: right;
  color: #fff;
  visibility: ${p => p.hide ? 'hidden' : 'visible'};
`

export const InputField = styled<{}, 'input'>('input')`
  display: inline-block;
  min-width: 215px;
  height: 30px;
  vertical-align: top;
  border: none;
  color: rgb(70, 70, 70);
  background: #000;
  border: 1px solid rgb(70, 70, 70);
  border-left: none;
  padding-left: 5px;

  &:focus {
    outline: 0;
  }
`

export const FiatInputField = styled(InputField)`
  color: #fff;
  border-top: none;
  border-left: none;
  border-right: 1px solid rgb(70, 70, 70);
  width: 75%;
  min-width: unset;
  border-left: none;
  padding-left: 10px;
  height: 29px;
  border-bottom: 1px solid rgb(70, 70, 70);
`

export const AssetDropdown = styled<StyleProps, 'div'>('div')`
  height: 30px;
  background: #000;
  color: #fff;
  border: 1px solid rgb(70, 70, 70);
  padding: 7px 3px 0px 8px;
  cursor: pointer;
  border-bottom: ${p => p.itemsShowing ? 'none' : '1px solid rgb(70, 70, 70)'};
`

export const AssetDropdownLabel = styled<{}, 'span'>('span')`
  float: left;
`

export const AssetItems = styled<StyleProps, 'div'>('div')`
  z-index: 1;
  background: #000;
  color: #fff;
  overflow-y: scroll;
  position: absolute;
  min-width: 254px;
  padding: 0px 8px;
  max-height: 75px;
  border: 1px solid rgb(70, 70, 70);
  border-top: none;
  height: ${p => p.isFiat ? 100 : 55}px;
  left: ${p => p.isFiat ? '15px' : 'auto'};
`

export const AssetItem = styled<StyleProps, 'div'>('div')`
  padding: 3px 0px;
  font-weight: bold;
  cursor: pointer;
  border-bottom: ${p => !p.isLast ? '1px solid rgb(70, 70, 70)' : ''};
`

export const ActionsWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 15px;
  text-align: center;
`

export const ConnectButton = styled<StyleProps, 'a'>('a')`
  font-size: 13px;
  font-weight: bold;
  border-radius: 20px;
  width: ${p => p.isSmall ? 50 : 100}%;
  background: #D9B227;
  border: 0;
  padding: 10px 70px;
  cursor: pointer;
  color: #000;
  margin-bottom: 10px;
  text-decoration: none;

  &:focus {
    outline: 0;
  }
`

export const BinanceIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 9px;
  margin-left: -4px;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 22px;
  font-weight: 600;
  color: #fff;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledTitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`

export const TLDSwitchWrapper = styled<StyleProps, 'div'>('div')`
  float: right;
  margin-top: -25px;
`

export const TLDSwitch = styled<StyleProps, 'div'>('div')`
  font-size: 13px;
  font-weight: bold;
  display: inline-block;
  margin-left: 10px;
  cursor: pointer;
  color: ${p => p.isActive ? '#F2C101' : '#9D7B01'};
`
