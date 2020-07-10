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

export const AssetQR = styled<{}, 'div'>('div')`
  float: right;
  margin-top: -3px;
  cursor: pointer;
`

export const DetailArea = styled<{}, 'div'>('div')`
  padding: 5px;
  font-weight: bold;
`

export const AssetLabel = styled<{}, 'span'>('span')`
  color: rgb(70, 70, 70);
  display: inline-block;
  font-weight: bold;
  font-size: 15px;
  min-width: 80px;
`

export const MemoArea = styled<{}, 'div'>('div')`
  padding: 5px;
`

export const MemoInfo = styled<{}, 'div'>('div')`
  float: left;
  max-width: 160px;
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

export const CopyButton = styled(GenButton)`
  float: right;
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
  margin-top: 3px;
`

export const NoticeWrapper = styled<{}, 'div'>('div')`
  padding-top: 75px;
  min-height: 250px;
  text-align: center;
  max-width: 240px;
`

export const SmallNoticeWrapper = styled(NoticeWrapper)`
  min-width: 244px;
`

export const QRImage = styled<{}, 'img'>('img')`
  width: 150px;
  height: 150px;
  display: block;
  margin: 0 auto 20px auto;
`

export const DetailLabel = styled<{}, 'span'>('span')`
  color: #7d7d7d;
  font-weight: bold;
  display: block;
  font-size: 15px;
  margin-bottom: 5px;
`

export const DetailIconWrapper = styled<{}, 'div'>('div')`
  float: right;
`

export const AccountSummary = styled(ListItem)`
  padding: 5px 7px;
`

export const ListInfo = styled<StyleProps, 'div'>('div')`
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

export const TradeLabel = styled<{}, 'span'>('span')`
  font-weight: bold;
  font-size: 14px;
  display: block;
  color: rgb(70, 70, 70);
  margin-bottom: 3px;
`

export const Balance = styled<StyleProps, 'span'>('span')`
  display: block;
  font-size: ${p => p.isSummary ? '25' : '14'}px;
  margin: ${p => p.isSummary ? '5px' : '20px'} 0;
  color: #fff;
  text-align: ${p => p.isSummary ? 'unset' : 'right'};
  margin-right: ${p => p.isSummary ? '0' : '7px'};
  -webkit-filter: blur(${p => p.hideBalance ? 10 : 0}px);
`

export const BlurIcon = styled<{}, 'div'>('div')`
  margin-left: 50%;
  margin-top: 25%;
  cursor: pointer;
  color: rgb(70, 70, 70);
`
