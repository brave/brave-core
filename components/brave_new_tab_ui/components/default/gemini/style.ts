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
  isSummary?: boolean
  isAsset?: boolean
  position?: string
  hideBalance?: boolean
  hide?: boolean
  itemsShowing?: boolean
  disabled?: boolean
  isSmall?: boolean
  isAuth?: boolean
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
  background: #000;
`

export const Header = styled('div')<{}>`
  text-align: left;
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

export const GeminiIcon = styled('div')<{}>`
  width: 27px;
  height: 27px;
  margin-right: 7px;
  margin-left: 2px;
`

export const StyledTitleText = styled('div')<{}>`
  margin-top: 4px;
`

export const DismissAction = styled('span')<{}>`
  display: block;
  cursor: pointer;
  color: #A6A6A6;
  font-size: 14px;
  margin-top: 20px;
  font-weight: bold;
`

export const IntroTitle = styled('span')<{}>`
  font-size: 14px;
  margin-top: 10px;
  display: block;
  font-weight: 600;
`

export const Copy = styled('p')<{}>`
  font-size: 13px;
  max-width: 240px;
  margin-top: 10px;
  margin-bottom: 20px;
  color: #A6A6A6;
`

export const ActionsWrapper = styled('div')<StyleProps>`
  margin-bottom: ${p => p.isAuth ? 20 : 5}px;
  text-align: center;
`

export const ConnectButton = styled('button')<StyleProps>`
  font-size: 14px;
  font-weight: bold;
  border-radius: 20px;
  width: 100%;
  background: #3D3D3D;
  border: 0;
  padding: 10px;
  cursor: pointer;
  color: #fff;
  text-decoration: none;
  min-width: 245px;

  &:focus {
    outline: 0;
  }
`

export const SmallButton = styled(ConnectButton)`
  width: 50%;
  margin-bottom: 10px;
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
  width: 33%;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  background: inherit;
  border: none;
  color: ${p => p.isActive ? '#EEEEEE' : '#8F8F8F'};
  padding: 0px;
  margin-left: 0px;
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
  padding: 6px 5px;
  overflow-y: auto;
  overflow-x: hidden;
`

export const ListIcon = styled('div')<{}>`
  margin-left: 5px;
  width: 28px;
  margin-top: 6px;
  float: left;
  margin-right: 10px;
`

export const ListImg = styled('img')<{}>`
  width: 20px;
  margin-top: -6px;
`

export const ListLabel = styled('div')<StyleProps>`
  color: #fff;
  cursor: ${p => p.clickable ? 'pointer' : 'initial'};
  margin-top: 10px;
  font-weight: 600;
`

export const AssetIconWrapper = styled('div')<StyleProps>`
  height: 25px;
  width: 25px;
  border-radius: 100px;
  display: inline-block;
`

export const AssetIcon = styled('span')<StyleProps>`
  margin-top: 6px;
  margin-left: 6px;
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

export const AssetQR = styled('div')<{}>`
  float: right;
  margin-top: -3px;
  cursor: pointer;
`

export const DetailArea = styled('div')<{}>`
  padding: 5px;
  font-weight: bold;
`

export const AssetLabel = styled('span')<{}>`
  color: rgb(70, 70, 70);
  display: inline-block;
  font-weight: bold;
  font-size: 15px;
  min-width: 80px;
`

export const MemoArea = styled('div')<{}>`
  padding: 5px;
`

export const MemoInfo = styled('div')<{}>`
  float: left;
  max-width: 160px;
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

export const CopyButton = styled(GenButton)`
  float: right;
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
  margin-top: 3px;
`

export const NoticeWrapper = styled('div')<{}>`
  padding-top: 75px;
  min-height: 250px;
  text-align: center;
  max-width: 240px;
`

export const SmallNoticeWrapper = styled(NoticeWrapper)`
  min-width: 244px;
`

export const QRImage = styled('img')<{}>`
  width: 150px;
  height: 150px;
  display: block;
  margin: 0 auto 20px auto;
`

export const DetailLabel = styled('span')<{}>`
  color: #7d7d7d;
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 5px;
`

export const DetailIconWrapper = styled('div')<{}>`
  float: right;
`

export const AccountSummary = styled(ListItem)`
  padding: 5px 7px;
`

export const ListInfo = styled('div')<StyleProps>`
  float: ${p => `${p.position}`};
  min-width: ${p => p.isSummary ? 60 : 83}px;
  font-size: ${p => p.isAsset ? '16px' : 'inherit'};
  margin-top: ${p => p.isAsset ? '9' : '0'}px;
  ${p => {
    if (p.position === 'right') {
      const width = p.isSummary ? 25 : 40
      return `
        width: ${width}%;
        text-align: left;
      `
    } else if (p.position === 'left') {
      return `width: 120px`
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

export const Balance = styled('span')<StyleProps>`
  display: block;
  font-size: ${p => p.isSummary ? '25' : '14'}px;
  margin: ${p => p.isSummary ? '5px' : '20px'} 0;
  color: #fff;
  text-align: ${p => p.isSummary ? 'unset' : 'right'};
  margin-right: ${p => p.isSummary ? '0' : '7px'};
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const BlurIcon = styled('div')<{}>`
  margin-left: 50%;
  margin-top: 25%;
  cursor: pointer;
  color: rgb(70, 70, 70);
`

export const TradeWrapper = styled('div')<{}>`
  margin-bottom: 20px;
`

export const InputWrapper = styled('div')<{}>`
  height: 30px;
  border: 1px solid white;
  margin-bottom: 10px;
`

export const InputField = styled('input')<{}>`
  display: inline-block;
  min-width: 215px;
  height: 30px;
  vertical-align: top;
  color: white;
  background: #000;
  padding-left: 5px;

  &:focus {
    outline: 0;
  }
`

export const AmountInputField = styled(InputField)`
  color: #fff;
  width: 70%;
  min-width: unset;
  padding-left: 10px;
  height: 29px;
  border-right: none;
  border-left: none;
  border-bottom: 1px solid white;
`

export const Dropdown = styled('div')<StyleProps>`
  float: right;
  width: 30%;
  padding: 7px 5px 0px 7px;
  border-left: none;
  cursor: ${p => p.disabled ? 'auto' : 'pointer'};
`

export const CaratDropdown = styled('div')<StyleProps>`
  width: 14px;
  height: 14px;
  float: right;
  color: #fff;
  visibility: ${p => p.hide ? 'hidden' : 'visible'};
`

export const AssetDropdown = styled('div')<StyleProps>`
  height: 30px;
  background: #000;
  color: #fff;
  border: 1px solid white;
  padding: 7px 3px 0px 8px;
  cursor: pointer;
  border-bottom: ${p => p.itemsShowing ? 'none' : '1px solid white'};
`
export const AssetDropdownLabel = styled('span')<{}>`
  font-weight: bold;
`

export const AssetItems = styled('div')<StyleProps>`
  z-index: 1;
  background: #000;
  color: #fff;
  overflow-y: scroll;
  position: absolute;
  min-width: 244px;
  padding: 0px 8px;
  max-height: 80px;
  border: 1px solid white;
  border-top: none;
  height: 80px;
  left: auto;
`

export const AssetItem = styled('div')<StyleProps>`
  padding: 5px 0px;
  font-weight: bold;
  cursor: pointer;
  border-bottom: ${p => !p.isLast ? '1px solid rgb(70, 70, 70)' : ''};
`

export const DropdownIcon = styled('span')<StyleProps>`
  height: 15px;
  float: left;
  margin-left: 5px;
  margin-right: 10px;
`

export const ActionButton = styled('button')<{}>`
  font-size: 13px;
  font-weight: bold;
  border-radius: 20px;
  width: 100%;
  background: #7CDDF9;
  border: 0;
  padding: 10px 60px;
  cursor: pointer;
  color: #000;
  margin-top: 20px;
`

export const TradeSwitchWrapper = styled('div')<StyleProps>`
  float: right;
  margin-top: -35px;
`

export const TradeSwitch = styled('div')<StyleProps>`
  font-size: 13px;
  font-weight: bold;
  display: inline-block;
  margin-left: 10px;
  cursor: pointer;
  color: ${p => p.isActive ? '#EEEEEE' : '#8F8F8F'};
`

export const DisconnectWrapper = styled('div')<{}>`
  padding-top: 75px;
  min-height: 250px;
  text-align: center;
  max-width: 240px;
`

export const Title = styled('span')<{}>`
  display: block;
  font-size: 13px;
  font-weight: bold;
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

export const DisconnectButton = styled(GenButton)`
  background: #AA1212;
  color: #fff;
  padding: 5px 20px;
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

export const TradeItemLabel = styled('span')<{}>`
  float: left;
  width: 45%;
  text-align: left;
  font-size: 15px;
`

export const TradeValue = styled('span')<{}>`
  font-weight: bold;
  float: right;
  width: 55%;
  text-align: right;
  font-size: 15px;
`

export const TradeInfoWrapper = styled('div')<StyleProps>`
  margin: 20px 0;
  overflow-y: auto;
`

export const TradeInfoItem = styled('div')<StyleProps>`
  margin: 5px 0;
  overflow-y: hidden;
  margin-top: ${p => p.isLast ? '15' : '5'}px;
`

export const StyledParty = styled('div')<{}>`
  font-size: 35px;
  margin: 10px 0px;
`
