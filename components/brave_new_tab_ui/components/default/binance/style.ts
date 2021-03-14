/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  hide?: boolean
  itemsShowing?: boolean
  isActive?: boolean
  isSmall?: boolean
  isLast?: boolean
  disabled?: boolean
  isFiat?: boolean
  active?: boolean
  isBTC?: boolean
  isAsset?: boolean
  isBuy?: boolean
  isLeading?: boolean
  isDetail?: boolean
  hideBalance?: boolean
  isFirstView?: boolean
  hideOverflow?: boolean
  userAuthed?: boolean
  clickable?: boolean
  position?: 'left' | 'right'
}

export const WidgetWrapper = styled('div')<StyleProps>`
  color: white;
  padding: 6px 20px 12px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  min-height: ${p => p.userAuthed ? '360px' : 'initial'};
  background-image: linear-gradient(140deg, #1F2327 0%, #000000 85%);
`

export const Header = styled('div')<{}>`
  text-align: left;
`

export const Content = styled('div')<{}>`
  margin: 10px 0;
  min-width: 255px;
`

export const Title = styled('span')<{}>`
  display: block;
  font-size: 13px;
  font-weight: bold;
`

export const Copy = styled('p')<{}>`
  font-size: 15px;
  max-width: 240px;
  margin-top: 20px;
  margin-bottom: 11px;
`

export const GenButton = styled('button')<{}>`
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

export const BuyPromptWrapper = styled('div')<{}>`
  margin-bottom: 20px;
`

export const FiatInputWrapper = styled('div')<{}>`
  height: 30px;
  border: 1px solid rgb(70, 70, 70);
  margin-bottom: 10px;
`

export const FiatDropdown = styled('div')<StyleProps>`
  float: right;
  width: ${p => p.isFiat ? 25 : 35}%;
  padding: 7px 5px 0px 7px;
  cursor: ${p => p.disabled ? 'auto' : 'pointer'};
`

export const CaratDropdown = styled('div')<StyleProps>`
  width: 14px;
  height: 14px;
  float: right;
  color: #fff;
  visibility: ${p => p.hide ? 'hidden' : 'visible'};
`

export const InputField = styled('input')<StyleProps>`
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
  width: ${p => p.isFiat ? 75 : 65}%;
  min-width: unset;
  border-left: none;
  padding-left: 10px;
  height: 29px;
  border-bottom: 1px solid rgb(70, 70, 70);
`

export const AssetDropdown = styled('div')<StyleProps>`
  height: 30px;
  background: #000;
  color: #fff;
  border: 1px solid rgb(70, 70, 70);
  padding: 7px 3px 0px 8px;
  cursor: pointer;
  border-bottom: ${p => p.itemsShowing ? 'none' : '1px solid rgb(70, 70, 70)'};
`

export const AssetDropdownLabel = styled('span')<{}>`
  float: left;
`

export const AssetItems = styled('div')<StyleProps>`
  z-index: 1;
  background: #000;
  color: #fff;
  overflow-y: scroll;
  position: absolute;
  min-width: 244px;
  border: 1px solid rgb(70, 70, 70);
  border-top: none;
  height: 95px;
  left: ${p => p.isFiat ? '20px' : 'auto'};
`

export const AssetItem = styled('div')<StyleProps>`
  padding: 5px 0 5px 5px;
  font-weight: bold;
  cursor: pointer;
  border-bottom: ${p => !p.isLast ? '1px solid rgb(70, 70, 70)' : ''};
`

export const ActionsWrapper = styled('div')<StyleProps>`
  margin-bottom: ${p => p.isFirstView ? 25 : 5}px;
  text-align: center;
`

export const ConnectButton = styled.a<StyleProps>`
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

export const ActionButton = styled.button<{}>`
  font-size: 13px;
  font-weight: bold;
  border-radius: 20px;
  width: 100%;
  background: #D9B227;
  border: 0;
  padding: 10px 55px;
  cursor: pointer;
  color: #000;
  margin-top: 20px;
`

export const BinanceIcon = styled('div')<{}>`
  width: 27px;
  height: 27px;
  margin-right: 11px;
  margin-left: -2px;
`

export const StyledTitle = styled('div')<{}>`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: #fff;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledTitleText = styled('div')<{}>`
  margin-top: 4px;
`

export const TLDSwitchWrapper = styled('div')<StyleProps>`
  float: right;
  margin-top: -25px;
`

export const TLDSwitch = styled('div')<StyleProps>`
  font-size: 13px;
  font-weight: bold;
  display: inline-block;
  margin-left: 10px;
  cursor: pointer;
  color: ${p => p.isActive ? '#F2C101' : '#9D7B01'};
`

export const NavigationBar = styled('div')<{}>`
  height: 30px;
  margin-top: 15px;

  &:first-child {
    margin-right: 3px;
  }
`

export const NavigationItem = styled('button')<StyleProps>`
  float: left;
  width: 25%;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  text-align: center;
  background: inherit;
  border: none;
  color: ${p => p.isActive ? '#F2C101' : '#9D7B01'};
  margin-right: ${p => p.isLeading ? 12 : 0}px
  margin-left: ${p => {
    if (p.isBuy) {
      return -12
    } else {
      return 0
    }
  }}px;

  &:focus {
    outline: 0;
  }
`

export const SelectedView = styled('div')<StyleProps>`
  border: 1px solid rgb(70, 70, 70);
  overflow-y: ${p => p.hideOverflow ? 'hidden' : 'scroll'};
  height: 260px;
  width: 240px;
  margin-left: 4px;
`

export const ListItem = styled('div')<{}>`
  border-bottom: 1px solid rgb(70, 70, 70);
  padding: 10px 5px;
  overflow-y: auto;
  overflow-x: hidden;
`

export const ListIcon = styled('div')<{}>`
  margin-left: 5px;
  width: 28px;
  margin-top: 6px;
  float: left;
`

export const AssetIcon = styled('span')<StyleProps>`
  margin-top: ${p => p.isDetail ? 5 : 0}px;
`

export const ListImg = styled('img')<{}>`
  width: 20px;
  margin-top: -6px;
`

export const QRImage = styled('img')<{}>`
  width: 150px;
  height: 150px;
  display: block;
  margin: 0 auto 20px auto;
`

export const ListLabel = styled('div')<StyleProps>`
  color: #fff;
  cursor: ${p => p.clickable ? 'pointer' : 'initial'};
  margin-top: 5px;
`

export const SearchInput = styled('input')<{}>`
  border: none;
  color: #fff;
  background: inherit;
  font-size: 15px;
  &:focus {
    outline: 0;
  }
`

export const DetailIcons = styled('div')<{}>`
  float: left;
  margin-top: -3px;
  margin-right: 10px;
`

export const AssetTicker = styled('span')<{}>`
  color: #fff;
  font-weight: bold;
  margin-right: 3px;
  font-size: 15px;
`

export const TickerLabel = styled('span')<{}>`
  font-size: 14px;
  font-weight bold;
  color: #fff;
`

export const AssetLabel = styled('span')<{}>`
  color: rgb(70, 70, 70);
  display: inline-block;
  font-weight: bold;
  font-size: 15px;
  min-width: 80px;
`

export const AssetQR = styled('div')<{}>`
  float: right;
  margin-top: -3px;
  cursor: pointer;
`

export const DetailArea = styled('div')<{}>`
  padding: 5px;
  font-weight: bold;
`

export const MemoArea = styled('div')<{}>`
  padding: 5px;
`

export const MemoInfo = styled('div')<{}>`
  float: left;
  max-width: 160px;
`

export const CopyButton = styled(GenButton)`
  float: right;
`

export const DetailLabel = styled('span')<{}>`
  color: #7d7d7d;
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 5px;
`

export const DetailInfo = styled('span')<{}>`
  color: #fff;
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 10px;
  width: 180px;
  word-wrap: break-word;
`

export const BackArrow = styled('div')<{}>`
  width: 20px;
  float: left;
  cursor: pointer;
  margin-right: 5px;
`

export const ListInfo = styled('div')<StyleProps>`
  float: ${p => `${p.position}`};
  min-width: ${p => p.isBTC ? 60 : 83}px;
  font-size: ${p => p.isAsset ? '16px' : 'inherit'};
  margin-top: ${p => p.isAsset ? '4' : '0'}px;

  ${p => {
    if (p.position === 'right') {
      const width = p.isBTC ? 25 : 40
      return `
        width: ${width}%;
        text-align: left;
      `
    } else {
      return null
    }
  }}
`

export const TradeLabel = styled('span')<{}>`
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

export const BuySellHeader = styled('div')<{}>`
  padding: 5px 0px;
  height: 45px;
  border-bottom: 1px solid rgb(70, 70, 70);
`

export const AssetInfo = styled('div')<{}>`
  float: left;
  color: #fff;
  text-align: left;
`

export const PairName = styled('span')<{}>`
  font-weight: bold;
  font-size: 14px;
  display: block;
`

export const PairPrice = styled('span')<{}>`
  font-size: 12px;
  display: block;
`

export const BuySellToggle = styled('div')<{}>`
  float: right;
`

export const Switch = styled('div')<StyleProps>`
  font-size: 14px;
  font-weight: bold;
  display: inline-block;
  border-radius: 8px;
  padding: ${p => p.active ? '7' : '6'}px 10px;
  background: ${p => p.active ? '#D9B227' : '#000'};
`

export const ActionLabel = styled('span')<{}>`
  color: rgb(70, 70, 70);
  display: block;
  padding: 5px 0px;
  font-weight: bold;
`

export const BuySellContent = styled('div')<{}>`
  padding: 5px 0px;
  text-align: center;
  min-width: 240px;
`

export const AmountInput = styled('div')<{}>`

`

export const AmountButton = styled('button')<{}>`
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

export const PercentWrapper = styled('div')<{}>`
  margin: 10px 0px;
`

export const Percent = styled('div')<{}>`
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

export const ConvertLabel = styled('span')<{}>`
  float: left;
  width: 45%;
  text-align: left;
  font-size: 15px;
`

export const ConvertValue = styled('span')<{}>`
  font-weight: bold;
  float: right;
  width: 55%;
  text-align: right;
  font-size: 15px;
`

export const ConvertInfoWrapper = styled('div')<StyleProps>`
  margin: 20px 0;
  overflow-y: auto;
`

export const ConvertInfoItem = styled('div')<StyleProps>`
  margin: 5px 0;
  overflow-y: hidden;
  margin-top: ${p => p.isLast ? '15' : '5'}px;
`

export const StyledEmoji = styled('div')<{}>`
  margin: 10px 0px;
`

export const DisconnectWrapper = styled('div')<{}>`
  padding-top: 75px;
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
  min-width: 244px;
`

export const DismissAction = styled('span')<{}>`
  display: block;
  cursor: pointer;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin-top: 20px;
  font-weight: bold;
`

export const ConnectAction = styled(DismissAction)`
  color: #ffffff;
  margin-bottom: -20px;
`

export const Balance = styled('span')<StyleProps>`
  display: block;
  font-size: ${p => p.isBTC ? '25' : '14'}px;
  font-weight bold;
  margin: 5px 0;
  color: #fff;
  text-align: ${p => p.isBTC ? 'unset' : 'right'};
  margin-right: ${p => p.isBTC ? '0' : '7px'};
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const Converted = styled('span')<StyleProps>`
  display: block;
  font-size: ${p => p.isBTC ? '16' : '14'}px;
  color: rgb(95, 95, 95);
  margin-left: ${p => p.isBTC ? 0 : 10}px;
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const BlurIcon = styled('div')<{}>`
  margin-left: 50%;
  margin-top: 25%;
  cursor: pointer;
  color: rgb(70, 70, 70);
`

export const DropdownIcon = styled('div')<StyleProps>`
  width: 12px;
  margin-right: 10px;
  display: inline-block;
`
