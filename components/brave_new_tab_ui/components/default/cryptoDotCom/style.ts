/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
interface StyleProps {
  as?: any
  $bg?: string
  center?: boolean
  clickable?: boolean
  chartWidth?: number
  chartHeight?: number
  column?: boolean
  disabled?: boolean
  flex?: number
  $fontSize?: number
  hasBorder?: boolean
  hasPadding?: boolean
  $hasSpacing?: boolean
  $height?: number
  hide?: boolean
  hideBalance?: boolean
  hideOverflow?: boolean
  href?: string
  inline?: boolean
  isActive?: boolean
  isAsset?: boolean
  isAuth?: boolean
  isDeposit?: boolean
  isDetail?: boolean
  isFlex?: boolean
  isFullWidth?: boolean
  isLast?: boolean
  isSmall?: boolean
  isSummary?: boolean
  itemsShowing?: boolean
  light?: boolean
  $m?: number | string
  $mt?: number | string
  $mr?: number | string
  $mb?: number | string
  $ml?: number | string
  onClick?: Function
  position?: string
  $p?: number | string
  $pt?: number | string
  $pr?: number | string
  $pb?: number | string
  $pl?: number | string
  showContent?: boolean
  small?: boolean
  large?: boolean
  style?: object
  textAlign?: string
  textColor?: string
  userAuthed?: boolean
  weight?: string | number
  marketView?: boolean
}

const colorNameToHex = {
  green: '#58B2A9',
  red: '#D986A2',
  light: 'rgba(255, 255, 255, 0.6)',
  xlight: 'rgba(255, 255, 255, 0.3)'
}

function getColor (p: any) {
  let colorName
  if (!/(string|undefined)/.test(typeof p)) {
    const keys = Object.keys(p)
    colorName = keys.find((key) => key in colorNameToHex)
  } else {
    colorName = p
  }
  return colorNameToHex[colorName]
}

export const Text = styled('p')<StyleProps>`
  color: ${p => getColor(p.textColor) || '#ffffff'};
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: ${p => (p.weight || (p.small ? '500' : 'normal'))};
  font-size: ${p => {
    return p.$fontSize || (p.small ? '11px' : (p.large ? '19px' : '14px'))
  }};
  margin: ${p => (p.$hasSpacing ? '20px 0px' : '0px')};
  text-align: ${p => (p.center ? 'center' : 'inherit')}
  display: ${p => (p.inline ? 'inline-block' : 'block')}

  ${(p) =>
    p.$p &&
    `padding: ${typeof p.$p === 'number' ? `${p.$p}px` : p.$p}`}
  ${(p) =>
    p.$pt &&
    `padding-top: ${typeof p.$pt === 'number' ? `${p.$pt}px` : p.$pt}`}
  ${(p) =>
    p.$pr &&
    `padding-right: ${typeof p.$pr === 'number' ? `${p.$pr}px` : p.$pr}`}
  ${(p) =>
    p.$pb &&
    `padding-bottom: ${typeof p.$pb === 'number' ? `${p.$pb}px` : p.$pb}`}
  ${(p) =>
    p.$pl &&
    `padding-left: ${typeof p.$pl === 'number' ? `${p.$pl}px` : p.$pl}`}

  ${(p) =>
    p.$m &&
    `margin: ${typeof p.$m === 'number' ? `${p.$m}px` : p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${typeof p.$mt === 'number' ? `${p.$mt}px` : p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${typeof p.$mr === 'number' ? `${p.$mr}px` : p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${typeof p.$mb === 'number' ? `${p.$mb}px` : p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${typeof p.$ml === 'number' ? `${p.$ml}px` : p.$ml}`}
 `

export const BasicBox = styled('div')<StyleProps>`
  ${(p) =>
    p.$p &&
    `padding: ${typeof p.$p === 'number' ? `${p.$p}px` : p.$p}`}
  ${(p) =>
    p.$pt &&
    `padding-top: ${typeof p.$pt === 'number' ? `${p.$pt}px` : p.$pt}`}
  ${(p) =>
    p.$pr &&
    `padding-right: ${typeof p.$pr === 'number' ? `${p.$pr}px` : p.$pr}`}
  ${(p) =>
    p.$pb &&
    `padding-bottom: ${typeof p.$pb === 'number' ? `${p.$pb}px` : p.$pb}`}
  ${(p) =>
    p.$pl &&
    `padding-left: ${typeof p.$pl === 'number' ? `${p.$pl}px` : p.$pl}`}

  ${(p) =>
    p.$m &&
    `margin: ${typeof p.$m === 'number' ? `${p.$m}px` : p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${typeof p.$mt === 'number' ? `${p.$mt}px` : p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${typeof p.$mr === 'number' ? `${p.$mr}px` : p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${typeof p.$mb === 'number' ? `${p.$mb}px` : p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${typeof p.$ml === 'number' ? `${p.$ml}px` : p.$ml}`}
 `

export const Box = styled(BasicBox)<StyleProps>`
  border: 1px solid rgba(79, 86, 97, 0.7);
  padding: ${p => (p.hasPadding ? '0.5em' : '0')};
  border-radius: 2px;
  display: ${p => (p.isFlex ? 'flex' : 'block')};
  height: ${p => (p.$height ? `${p.$height}px` : 'auto')};

   ${(p) =>
     p.isFlex &&
     `
      flex-direction: ${p.column ? 'column' : 'row'};
      justify-content: space-between;
      align-items: center;
    `}
 `

export const FlexItem = styled('div')<StyleProps>`
  display: ${p => (p.isFlex ? 'flex' : 'block')};
  flex: ${p => p.flex || 'inherit'};
  text-align: ${p => p.textAlign || 'left'};
  padding: ${p => (p.hasPadding ? '0.5em' : '0px')};
  width: ${p => (p.isFullWidth ? '100%' : 'auto')};
  border-bottom: ${p => (p.hasPadding ? '1px solid rgba(79, 86, 97, 0.7)' : 'none')};

  ${(p) =>
    p.isFlex &&
    `
     flex-direction: ${p.column ? 'column' : 'row'};
     justify-content: space-between;
     align-items: center;
   `}

  ${(p) =>
    p.$pl &&
    `padding-left: ${typeof p.$pl === 'number' ? `${p.$pl}px` : p.$pl}`}
  ${(p) =>
    p.$pr &&
    `padding-right: ${typeof p.$pr === 'number' ? `${p.$pr}px` : p.$pr}`}
 `

export const PlainButton = styled('button')<StyleProps>`
  display: ${p => p.inline ? 'inline-block' : 'block'};
  background: none;
  border: none;
  cursor: pointer;
  color: ${p => getColor(p.textColor) || '#ffffff'};

  &:focus {
    outline: 0;
  }

  ${(p) =>
    p.$m &&
    `margin: ${typeof p.$m === 'number' ? `${p.$m}px` : p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${typeof p.$mt === 'number' ? `${p.$mt}px` : p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${typeof p.$mr === 'number' ? `${p.$mr}px` : p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${typeof p.$mb === 'number' ? `${p.$mb}px` : p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${typeof p.$ml === 'number' ? `${p.$ml}px` : p.$ml}`}
 `

export const WidgetWrapper = styled('div')<StyleProps>`
  color: white;
  padding: 6px 20px 30px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  overflow: hidden;
  min-width: 284px;
  background: rgba(15, 28, 45, 0.7);
  backdrop-filter: blur(16px);
 `

export const Header = styled('div')<StyleProps>`
  text-align: left;
  margin-bottom: ${p => (p.showContent ? '15' : '0')}px;
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

export const CryptoDotComIcon = styled('div')<{}>`
  width: 27px;
  height: 27px;
  margin-right: 7px;
  margin-left: 2px;

  & svg {
    width: 100%;
    height: 100%;
  }
 `

export const StyledTitleText = styled('div')<{}>``

export const List = styled(Box)<StyleProps>`
  overflow-y: ${p => (p.hideOverflow ? 'hidden' : 'scroll')};
  height: 260px;
  padding: 0;
  margin: 0;
 `

export const ListItem = styled('li')<StyleProps>`
  border-bottom: 1px solid rgba(79, 86, 97, 0.7);
  padding: 5px;
  border-radius: 2px;
  display: ${p => (p.isFlex ? 'flex' : 'block')};
  cursor: ${p => (p.onClick ? 'pointer' : 'initial')}
  height: ${p => (p.$height ? p.$height : 'auto')};
  ${(p) =>
    p.isFlex && `
    justify-content: space-between;
    align-items: center;
  `};
`

export const BackArrow = styled('div')<StyleProps>`
  width: 20px;
  cursor: pointer;
  margin-left: ${p => p.marketView ? 60 : 0}px;
 `

export const ActionButton = styled('button')<StyleProps>`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: ${p => (p.small ? '13px' : '15px')};
  font-weight: ${p => (p.small ? '500' : 'bold')};
  border-radius: 20px;
  width: ${p => (p.inline ? 'auto' : '100%')};
  background: ${p => (p.light ? 'rgba(255, 255, 255, 0.21)' : '#44B0FF')};
  border: 0;
  padding: ${p => (p.small ? '6px 10px' : '10px 0px')};
  cursor: pointer;
  color: #ffffff;
  line-height: 1;
  text-transform: uppercase;

  ${(p) =>
    p.$m &&
    `margin: ${typeof p.$m === 'number' ? `${p.$m}px` : p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${typeof p.$mt === 'number' ? `${p.$mt}px` : p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${typeof p.$mr === 'number' ? `${p.$mr}px` : p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${typeof p.$mb === 'number' ? `${p.$mb}px` : p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${typeof p.$ml === 'number' ? `${p.$ml}px` : p.$ml}`}
 `

export const ActionAnchor = styled('span')<StyleProps>`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: ${p => (p.small ? '13px' : '15px')};
  font-weight: ${p => (p.small ? '500' : 'bold')};
  border-radius: 20px;
  width: ${p => (p.inline ? 'auto' : '100%')};
  background: ${p => (p.light ? 'rgba(255, 255, 255, 0.21)' : '#44B0FF')};
  border: 0;
  padding: ${p => (p.small ? '6px 10px' : '10px 0px')};
  margin: 10px 0 15px;
  cursor: pointer;
  color: #ffffff;
  line-height: 1;
  display: block;
  text-align: center;
  text-decoration: none;
 `

export const UpperCaseText = styled('span')<StyleProps>`
  text-transform: uppercase;
`
export const SVG = styled('svg')<StyleProps>`
  margin: 1rem 0;
`
