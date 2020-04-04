/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  hide?: boolean
  itemsShowing?: boolean
  isActive?: boolean
  isSmall?: boolean
  isInTab?: boolean
  isLast?: boolean
  disabled?: boolean
  isFiat?: boolean
  active?: boolean
  isBTC?: boolean
  isAsset?: boolean
  isBuy?: boolean
  isSummary?: boolean
  isDetail?: boolean
  hideBalance?: boolean
  position?: 'left' | 'right'
}

export const WidgetWrapper = styled<{}, 'div'>('div')`
  color: white;
  padding: 10px 15px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  background-image: linear-gradient(140deg, #1F2327 0%, #000000 85%);
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

export const GenButton = styled<{}, 'button'>('button')`
  font-size: 13px;
  font-weight: bold;
  border-radius: 20px;
  border: 0;
  padding: 5px 10px;
  cursor: pointer;
  background: #2C2C2B;
  color: rgba(255, 255, 255, 0.7);
`

export const DisconnectButton = styled(GenButton)`
  background: #AA1212;
  color: #fff;
  padding: 5px 20px;
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
  margin-bottom: 5px;
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

export const ConvertButton = styled<{}, 'button'>('button')`
  font-size: 13px;
  font-weight: bold;
  border-radius: 20px;
  width: 100%;
  background: #D9B227;
  border: 0;
  padding: 10px 65px;
  cursor: pointer;
  color: #000;
  margin-bottom: -10px;
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

export const ActionTray = styled<{}, 'div'>('div')`
  float: right;
  margin-top: -19px;
  margin-right: 75px;
  display: inline-block;

  > *:nth-child(1) {
    margin-right: 20px;
  }
`

export const ActionItem = styled<{}, 'div'>('div')`
  cursor: pointer;
  display: inline-block;
  vertical-align: middle;
`

export const ConnectPrompt = styled<{}, 'div'>('div')`
  float: right;
  margin-top: -20px;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
`

export const NavigationBar = styled<{}, 'div'>('div')`
  height: 30px;
  margin-top: 10px;
`

export const NavigationItem = styled<StyleProps, 'div'>('div')`
  float: left;
  width: 25%;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  text-align: center;
  color: ${p => p.isActive ? '#F2C101' : '#9D7B01'};
  margin-left: ${p => {
    if (p.isBuy) {
      return -6
    } else if (p.isSummary) {
      return 5
    } else {
      return 0
    }
  }}px;
`

export const SelectedView = styled<{}, 'div'>('div')`
  border: 1px solid rgb(70, 70, 70);
  overflow-y: scroll;
  height: 170px;
  width: 250px;
  margin-left: 4px;
`

export const ListItem = styled<{}, 'div'>('div')`
  border-bottom: 1px solid rgb(70, 70, 70);
  padding: 10px 5px;
  overflow-y: auto;
  overflow-x: hidden;
`

export const ListIcon = styled<{}, 'div'>('div')`
  margin-left: 5px;
  width: 28px;
  margin-top: 6px;
  float: left;
`

export const AssetIcon = styled<StyleProps, 'span'>('span')`
  margin-top: ${p => p.isDetail ? 5 : 0}px;
`

export const ListImg = styled<{}, 'img'>('img')`
  width: 20px;
  margin-top: -6px;
`

export const ListLabel = styled<{}, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  margin-top: 5px;
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

export const DetailIcons = styled<{}, 'div'>('div')`
  float: left;
  margin-top: -3px;
  margin-right: 10px;
`

export const AssetTicker = styled<{}, 'span'>('span')`
  color: #fff;
  font-weight: bold;
  margin-right: 3px;
  font-size: 15px;
`

export const TickerLabel = styled<{}, 'span'>('span')`
  font-size: 14px;
  font-weight bold;
  color: #fff;
`

export const AssetLabel = styled<{}, 'span'>('span')`
  color: rgb(70, 70, 70);
  display: inline-block;
  font-weight: bold;
  font-size: 15px;
  min-width: 80px;
`

export const AssetQR = styled<{}, 'div'>('div')`
  float: right;
  margin-top: -3px;
`

export const DetailArea = styled<{}, 'div'>('div')`
  padding: 5px;
  font-weight: bold;
`

export const MemoArea = styled<{}, 'div'>('div')`
  padding: 5px;
`

export const MemoInfo = styled<{}, 'div'>('div')`
  float: left;
  max-width: 110px;
`

export const CopyButton = styled(GenButton)`
  float: right;
`

export const DetailLabel = styled<{}, 'span'>('span')`
  color: rgb(70, 70, 70);
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 5px;
`

export const DetailInfo = styled<{}, 'span'>('span')`
  color: #fff;
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 10px;
  width: 180px;
  word-wrap: break-word;
`

export const BackArrow = styled<{}, 'div'>('div')`
  width: 20px;
  float: left;
  cursor: pointer;
  margin-right: 5px;
`

export const ListInfo = styled<StyleProps, 'div'>('div')`
  float: ${p => `${p.position}`};
  min-width: ${p => p.isBTC ? 70 : 83}px;
  font-size: ${p => p.isAsset ? '16px' : 'inherit'};
  margin-top: ${p => p.isAsset ? '4' : '0'}px;

  ${p => {
    if (p.position === 'right') {
      const width = p.isBTC ? 33 : 40
      return `
        width: ${width}%;
        text-align: left;
      `
    } else {
      return null
    }
  }}
`

export const TradeLabel = styled<{}, 'span'>('span')`
  font-weight: bold;
  font-size: 14px;
  display: block;
  color: rgb(70, 70, 70);
  margin-bottom: 3px;
`

export const BTCValueLabel = styled(TradeLabel)`
  color: #6DD401;
`

export const OtherValueLabel = styled(TradeLabel)`
  color: #fff;
`

export const BTCSummary = styled(ListItem)`
  padding: 5px 7px;
`

export const BuySellHeader = styled<{}, 'div'>('div')`
  padding: 5px 0px;
  height: 45px;
  border-bottom: 1px solid rgb(70, 70, 70);
`

export const AssetInfo = styled<{}, 'div'>('div')`
  float: left;
  color: #fff;
  text-align: left;
`

export const PairName = styled<{}, 'span'>('span')`
  font-weight: bold;
  font-size: 14px;
  display: block;
`

export const PairPrice = styled<{}, 'span'>('span')`
  font-size: 12px;
  display: block;
`

export const BuySellToggle = styled<{}, 'div'>('div')`
  float: right;
`

export const Switch = styled<StyleProps, 'div'>('div')`
  font-size: 14px;
  font-weight: bold;
  display: inline-block;
  border-radius: 8px;
  padding: ${p => p.active ? '7' : '6'}px 10px;
  background: ${p => p.active ? '#D9B227' : '#000'};
`

export const ActionLabel = styled<{}, 'span'>('span')`
  color: rgb(70, 70, 70);
  display: block;
  padding: 5px 0px;
  font-weight: bold;
`

export const BuySellContent = styled<{}, 'div'>('div')`
  padding: 5px 0px;
  text-align: center;
  min-width: 240px;
`

export const AmountInput = styled<{}, 'div'>('div')`

`

export const AmountButton = styled<{}, 'button'>('button')`
  font-size: 16px;
  font-weight: bold;
  border-radius: 4px;
  border: 0;
  padding: 5px;
  cursor: pointer;
  background: #2C2C2B;
  color: #fff;
  width: 13%;
  height: 30px;
  display: inline-block;
`

export const AmountTextInput = styled(InputField)`
  min-width: unset;
  vertical-align: unset;
  width: 65%;
  margin: 0px 5px;
  color: #fff;
  border-left: 1px solid rgb(70, 70, 70);
`

export const PercentWrapper = styled<{}, 'div'>('div')`
  margin: 10px 0px;
`

export const Percent = styled<{}, 'div'>('div')`
  padding: 2px 5px;
  color: #fff;
  border: 1px solid rgb(70, 70, 70);
  margin-right: 1px;
  border-radius: 3px;
  cursor: pointer;
  display: inline-block;
`

export const BuySellButton = styled(ConnectButton)`
  padding: 5px;
  margin: 5px 0px;
  background: ${p => p.isBuy ? '#3BB260' : '#DD5353'};
`

export const AvailableLabel = styled<{}, 'span'>('span')`
  float: right;
  margin-top: -27px;
  font-size: 13px;
`

export const ConvertLabel = styled<{}, 'span'>('span')`
  float: left;
  width: 45%;
  text-align: left;
  font-size: 15px;
`

export const ConvertValue = styled<{}, 'span'>('span')`
  font-weight: bold;
  float: right;
  width: 55%;
  text-align: right;
  font-size: 15px;
`

export const ConvertInfoWrapper = styled<StyleProps, 'div'>('div')`
  margin: 10px 0;
  overflow-y: auto;
`

export const ConvertInfoItem = styled<StyleProps, 'div'>('div')`
  margin: 5px 0;
  overflow-y: auto;
  margin-top: ${p => p.isLast ? '15' : '5'}px;
`

export const StyledEmoji = styled<{}, 'div'>('div')`
  margin: 10px 0px;
`

export const DisconnectWrapper = styled<{}, 'div'>('div')`
  padding-top: 35px;
  min-height: 250px;
  text-align: center;
  max-width: 240px;
`

export const DisconnectTitle = styled(Title)`
  font-size: 14px;
  max-width: 245px;
  margin: 0 auto;
  line-height: 18px;
`

export const DisconnectCopy = styled(Copy)`
  color: #fff;
  max-width: 220px;
  line-height: 17px;
  margin: 8px auto 15px auto;
`

export const InvalidTitle = styled(DisconnectTitle)`
  max-width: unset;
  margin-bottom: 20px;
`

export const InvalidCopy = styled(DisconnectCopy)`
  max-width: 210px;
`

export const InvalidWrapper = styled(DisconnectWrapper)`
  min-width: 255px;
`

export const DismissAction = styled<{}, 'span'>('span')`
  display: block;
  cursor: pointer;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin-top: 20px;
  font-weight: bold;
`

export const Balance = styled<StyleProps, 'span'>('span')`
  display: block;
  font-size: ${p => p.isBTC ? '25' : '14'}px;
  font-weight bold;
  margin: 5px 0;
  color: #fff;
  text-align: ${p => p.isBTC ? 'unset' : 'right'};
  margin-right: ${p => p.isBTC ? '0' : '7px'};
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const Converted = styled<StyleProps, 'span'>('span')`
  display: block;
  font-size: ${p => p.isBTC ? '16' : '14'}px;
  color: rgba(70, 70, 70);
  margin-left: ${p => p.isBTC ? 0 : 10}px;
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const BlurIcon = styled<{}, 'div'>('div')`
  margin-left: 60%;
  margin-top: 25%;
  cursor: pointer;
  color: rgb(70, 70, 70);
`
