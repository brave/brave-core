/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  itemsShowing?: boolean
  isSmall?: boolean
  isInTab?: boolean
}

export const StyledTitleTab = styled<StyleProps, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  padding: 10px ${p => p.isInTab ? 13 : 25}px;
  margin-bottom: -3px;
  border-radius: 6px 6px 0 0;
  backdrop-filter: blur(75px);
`

export const WidgetWrapper = styled<{}, 'div'>('div')`
  color: white;
  padding: 10px 15px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow-x: hidden;
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
  margin-bottom: 15px;
`

export const FiatInputWrapper = styled<{}, 'div'>('div')`
  height: 30px;
  border: 1px solid rgb(70, 70, 70);
  margin-bottom: 10px;
`
export const FiatDropdown = styled<{}, 'div'>('div')`
  float: right;
  width: 25%;
  padding: 7px 3px 0px 7px;
`

export const CaratDropdown = styled<{}, 'div'>('div')`
  width: 18px;
  height: 18px;
  float: right;
  margin-top: -2px;
  color: #fff;
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
  border-bottom: 1px solid rgb(70, 70, 70);;
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

export const AssetItems = styled<{}, 'div'>('div')`
  background: #000;
  color: #fff;
  height: 100px;
  overflow-y: scroll;
  position: absolute;
  width: 240px;
  padding: 0px 8px;
  max-height: 75px;
  border: 1px solid rgb(70, 70, 70);
  border-top: none;
`

export const ActionsWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 15px;
  text-align: center;
`

export const ConnectButton = styled<StyleProps, 'button'>('button')`
  font-size: 16px;
  font-weight: bold;
  border-radius: 20px;
  width: ${p => p.isSmall ? 50 : 100}%;
  background: #D9B227;
  border: 0;
  padding: 10px 0;
  cursor: pointer;
  color: #000;
  margin-bottom: 10px;
`

export const ExchangeIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 9px;
  margin-left: 8px;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 22px;
  font-weight: 600;
  color: #fff;
  font-family: Poppins, sans-serif;
`

export const StyledTitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`
