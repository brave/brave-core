/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'

interface StyleProps {
  isFiat?: boolean
  contentShowing?: boolean
  dropdownShowing?: boolean
}

export const WidgetWrapper = styled<StyleProps, 'div'>('div')`
  padding: 6px 20px 12px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  background: #fff;
`

export const Header = styled<{}, 'div'>('div')`
  text-align: left;
`

export const Title = styled<StyleProps, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: ${p => p.contentShowing ? '#000' : '#fff'};
  font-family: ${p => p.theme.fontFamily.heading};
`

export const BitcoinDotComIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 7px;
  margin-left: 2px;
`

export const TitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`

export const InputHeader = styled<{}, 'div'>('div')`
  margin-top: 15px;
`

export const InputLabel = styled<{}, 'span'>('span')`
  color: #7E8496;
  font-weight: bold;
`

export const Dropdown = styled<StyleProps, 'div'>('div')`
  border: 1px solid lightgray;
  border-radius: 6px;
  margin-top: 5px;
  cursor: pointer;

  ${(p) => p.dropdownShowing
    ? css`
      border-bottom: none;
      border-bottom-left-radius: 0px;
      border-bottom-right-radius: 0px;
    ` : ''
  }
`

export const AssetItem = styled<{}, 'div'>('div')`
  padding: 7px;
  cursor: pointer;
`

export const AssetImage = styled<{}, 'img'>('img')`
  width: 24px;
`

export const AssetTitle = styled<{}, 'span'>('span')`
  font-weight: bold;
  font-size: 14px;
  margin: 0px 5px;
  vertical-align: super;
`

export const AssetSymbol = styled<StyleProps, 'span'>('span')`
  font-size: 12px;
  border-radius: 4px;
  background: lightgray;
  padding: 2px 7px;
  margin-left: 10px;
  vertical-align: super;

  ${(p) => p.dropdownShowing
    ? css`
      float: right;
      margin-top: 5px;
      min-width: 40px;
      text-align: center;
    ` : ''
  }
`

export const DropdownIcon = styled<StyleProps, 'div'>('div')`
  width: 15px;
  float: right;
  margin-top: ${p => p.isFiat ? 1 : -28}px;
  margin-right: ${p => p.isFiat ? 3 : 10}px;
`

export const CurrencyItems = styled<{}, 'div'>('div')`
  z-index: 1;
  background: #fff;
  overflow-y: scroll;
  position: absolute;
  min-width: 244px;
  max-height: 155px;
  border: 1px solid lightgray;
  border-top: none;
  left: auto;
  border-bottom-left-radius: 6px;
  border-bottom-right-radius: 6px;
`

export const AmountInputWrapper = styled<{}, 'div'>('div')`
  margin-top: 5px;
`

export const AmountInput = styled<StyleProps, 'input'>('input')`
  font-weight: bold;
  height: 40px;
  border: 1px solid lightgray;
  border-radius: 6px;
  font-size: 15px;
  padding-left: 10px;
  border-right: none;
  border-top-right-radius: 0px;
  border-bottom-right-radius: 0px;

  ${(p) => p.dropdownShowing
    ? css`
      border-bottom-left-radius: 0px;
    ` : ''
  }
`

export const FiatLabel = styled<{}, 'span'>('span')`
  color: #000;
  font-weight: bold;
  margin-left: 8px;
  font-size: 15px;
`

export const FiatDropdown = styled<StyleProps, 'div'>('div')`
  border-radius: 6px;
  border: 1px solid lightgray;
  display: inline-block;
  height: 40px;
  vertical-align: bottom;
  width: 64px;
  padding-top: 10px;
  cursor: pointer;
  border-top-left-radius: 0px;
  border-bottom-left-radius: 0px;

  ${(p) => p.dropdownShowing
    ? css`
      border-bottom: none;
      border-bottom-right-radius: 0px;
    ` : ''
  }
`

export const FiatItems = styled<{}, 'div'>('div')`
  z-index: 1;
  background: #fff;
  overflow-y: scroll;
  position: absolute;
  min-width: 244px;
  max-height: 90px;
  border: 1px solid lightgray;
  border-top: none;
  left: auto;
  border-bottom-left-radius: 6px;
  border-bottom-right-radius: 6px;
`

export const FiatItem = styled<{}, 'div'>('div')`
  color: #000;
  padding: 7px 0px 7px 7px;
  cursor: pointer;
`

export const FiatSymbol = styled<{}, 'div'>('div')`
  font-size: 12px;
  border-radius: 4px;
  background: lightgray;
  padding: 2px 5px 2px 7px;
  display: inline-block;
  min-width: 40px;
`

export const FiatName = styled<{}, 'span'>('span')`
  font-size: 13px;
  font-weight: bold;
  margin-left: 8px;
`

export const FooterWrapper = styled<{}, 'div'>('div')`
  margin-top: 15px;
`

export const BuyButton = styled<{}, 'button'>('button')`
  width: 100%;
  border-radius: 15px;
  border: none;
  background: #00D68F;
  color: white;
  padding: 12px 0px;
  font-weight: bold;
  font-size: 13px;
  cursor: pointer;
`

export const FooterInfo = styled<{}, 'div'>('div')`
  font-size: 10px;
  text-align: center;
  margin-top: 15px;
`

export const LinkLabel = styled<{}, 'span'>('span')`
  font-weight: bold;
  color: #000;
  margin-right: 5px;
`

export const Link = styled<{}, 'span'>('span')`
  font-weight: bold;
  color: #00C985;
  text-decoration: underline;
  cursor: pointer;
`
