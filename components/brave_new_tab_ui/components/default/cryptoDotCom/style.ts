/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
interface StyleProps {
  as?: any
  clickable?: boolean
  $color?: string
  disabled?: boolean
  flex?: number
  hasBorder?: boolean
  hide?: boolean
  hideBalance?: boolean
  hideOverflow?: boolean
  light?: boolean
  isActive?: boolean
  isAsset?: boolean
  isAuth?: boolean
  isDeposit?: boolean
  isDetail?: boolean
  isFlex?: boolean
  isLast?: boolean
  isSmall?: boolean
  isSummary?: boolean
  itemsShowing?: boolean
  $hasSpacing?: boolean
  noPadding?: boolean
  position?: string
  showContent?: boolean
  small?: boolean
  textAlign?: string
  textColor?: string
  userAuthed?: boolean
  onClick?: Function
}

const colorNameToHex = {
  green: "#58B2A9",
  red: "#D986A2",
  light: "rgba(255, 255, 255, 0.6)"
};

function getColor(p: any) {
  let colorName;
  if (!/(string|undefined)/.test(typeof p)) {
    const keys = Object.keys(p);
    colorName = keys.find((key) => key in colorNameToHex);
  } else {
    colorName = p;
  }
  return colorNameToHex[colorName];
}

export const Text = styled<StyleProps, 'p'>('p')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: ${p => p.small ? "500" : "normal"};
  color: ${p => getColor(p.$color) || "#ffffff"};
  font-size: ${p => p.small ? "11px" : "14px"};
  margin: ${p => p.$hasSpacing ? "20px 0px" : "0px"};
`;

export const Box = styled<StyleProps, 'div'>('div')`
  border: 1px solid #979797;
  padding: ${p => p.noPadding ? "0" : "5px"};
  border-radius: 2px;
  display: ${(p) => (p.isFlex ? "flex" : "block")};

  ${p => p.isFlex && `
    justify-content: space-between;
    align-items: center;
  `}
`;

export const FlexItem = styled<StyleProps, 'div'>('div')`
  flex: ${p => p.flex || "inherit"};
  text-align: ${p => p.textAlign || "left"}
`;

export const PlainButton = styled<StyleProps, 'button'>('button')`
  background: none;
  border: none;
  cursor: pointer;
  color: ${(p) => getColor(p.textColor) || "#ffffff"};

  &:focus {
    outline: 0;
  }
`;

export const WidgetWrapper = styled<StyleProps, 'div'>('div')`
  color: white;
  padding: 6px 20px 30px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
  overflow: hidden;
  min-width: 284px;
  background: rgba(15, 28, 45, 0.7);
  backdrop-filter: blur(16px);
`

export const Header = styled<StyleProps, 'div'>('div')`
  text-align: left;
  margin-bottom: ${p => p.showContent ? '15' : '0'}px;
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

export const CryptoDotComIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 7px;
  margin-left: 2px;
`

export const StyledTitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`

export const List = styled(Box)`
  overflow-y: ${p => p.hideOverflow ? 'hidden' : 'scroll'};
  height: 260px;
  padding: 0;
  margin: 0;
`

export const ListItem = styled<StyleProps, 'li'>('li')`
  border-bottom: 1px solid #979797;
  padding: 5px;
  border-radius: 2px;
  display: ${p => p.isFlex ? "flex" : "block"};
  cursor: ${p => p.onClick ? "pointer" : "initial"}
  
  ${p => p.isFlex && `
    justify-content: space-between;
    align-items: center;
  `}
`;

export const AssetIconWrapper = styled<StyleProps, 'div'>('div')`
  height: 18px;
  width: 18px;
  border-radius: 100px;
  display: inline-block;
`

export const AssetIcon = styled<StyleProps, 'span'>('span')`
  font-size: 0.9em;
  margin-top: 0.2em;
  margin-left: 0.2em;
`

export const BackArrow = styled<{}, 'div'>('div')`
  width: 20px;
  cursor: pointer;
`

export const ActionButton = styled<StyleProps, 'button'>('button')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: ${p => p.small ? "13px" : "15px"};
  font-weight: ${p => p.small ? "500" : "bold"};
  border-radius: 20px;
  width: 100%;
  background: ${p => p.light ? "rgba(255, 255, 255, 0.21)" : "#44B0FF"};
  border: 0;
  padding: ${p => p.small ? "6px 10px" : "10px 0px"};
  cursor: pointer;
  color: #FFFFFF;
  line-height: 1;
`