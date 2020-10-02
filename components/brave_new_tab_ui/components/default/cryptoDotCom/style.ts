/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { StyledInterface } from 'styled-components'
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

export const Text = styled<StyleProps, 'p'>('p')`
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
    `padding: ${p.$p}`}
  ${(p) =>
    p.$pt &&
    `padding-top: ${p.$pt}`}
  ${(p) =>
    p.$pr &&
    `padding-right: ${p.$pr}`}
  ${(p) =>
    p.$pb &&
    `padding-bottom: ${p.$pb}`}
  ${(p) =>
    p.$pl &&
    `padding-left: ${p.$pl}`}

  ${(p) =>
    p.$m &&
    `margin: ${p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${p.$ml}`}
 `

export const BasicBox = styled<StyleProps, 'div'>('div')`
  ${(p) =>
    p.$p &&
    `padding: ${p.$p}`}
  ${(p) =>
    p.$pt &&
    `padding-top: ${p.$pt}`}
  ${(p) =>
    p.$pr &&
    `padding-right: ${p.$pr}`}
  ${(p) =>
    p.$pb &&
    `padding-bottom: ${p.$pb}`}
  ${(p) =>
    p.$pl &&
    `padding-left: ${p.$pl}`}

  ${(p) =>
    p.$m &&
    `margin: ${p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${p.$ml}`}
 `

export const Box = styled<StyleProps, StyledInterface>(BasicBox)`
  border: 1px solid rgba(79, 86, 97, 0.7);
  padding: ${p => (p.hasPadding ? '0.5em' : '0')};
  border-radius: 2px;
  display: ${p => (p.isFlex ? 'flex' : 'block')};
  height: ${p => (p.$height ? p.$height : 'auto')};

   ${(p) =>
     p.isFlex &&
     `
      flex-direction: ${p.column ? 'column' : 'row'};
      justify-content: space-between;
      align-items: center;
    `}
 `

export const FlexItem = styled<StyleProps, 'div'>('div')`
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
    `padding-left: ${p.$pl}`}
  ${(p) =>
    p.$pr &&
    `padding-left: ${p.$pr}`}
 `

export const PlainButton = styled<StyleProps, 'button'>('button')`
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
    `margin: ${p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${p.$ml}`}
 `

export const WidgetWrapper = styled<StyleProps, 'div'>('div')`
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

export const Header = styled<StyleProps, 'div'>('div')`
  text-align: left;
  margin-bottom: ${p => (p.showContent ? '15' : '0')}px;
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

  & svg {
    width: 100%;
    height: 100%;
  }
 `

export const StyledTitleText = styled<{}, 'div'>('div')``

export const List = styled(Box)`
  overflow-y: ${p => (p.hideOverflow ? 'hidden' : 'scroll')};
  height: 260px;
  padding: 0;
  margin: 0;
 `

export const ListItem = styled<StyleProps, 'li'>('li')`
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

export const AssetIconWrapper = styled<StyleProps, 'div'>('div')`
  height: 18px;
  width: 18px;
  border-radius: 100px;
  display: inline-block;
  cursor: ${p => (p.onClick ? 'pointer' : 'initial')}
  height: ${p => (p.$height ? p.$height : 'auto')};
  background: ${p => (p.$bg ? p.$bg : '#fff')};
  color: ${p => (p.textColor ? p.textColor : '#000')};
 `

export const AssetIcon = styled<StyleProps, 'span'>('span')`
  font-size: 0.9em;
  margin-top: 0.3em;
  margin-left: 0.2em;
 `

export const BackArrow = styled<{}, 'div'>('div')`
  width: 20px;
  cursor: pointer;
 `

export const ActionButton = styled<StyleProps, 'button'>('button')`
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
    `margin: ${p.$m}`}
  ${(p) =>
    p.$mt &&
    `margin-top: ${p.$mt}`}
  ${(p) =>
    p.$mr &&
    `margin-right: ${p.$mr}`}
  ${(p) =>
    p.$mb &&
    `margin-bottom: ${p.$mb}`}
  ${(p) =>
    p.$ml &&
    `margin-left: ${p.$ml}`}
 `

export const ActionAnchor = styled<StyleProps, 'span'>('span')`
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
  display: block;
  text-align: center;
  text-decoration: none;
 `

export const UpperCaseText = styled<StyleProps, 'span'>('span')`
  text-transform: uppercase;
`
export const SVG = styled<StyleProps, 'svg'>('svg').attrs(props => ({
  viewBox: `0 0 ${props.chartWidth} ${props.chartHeight}`
}))`
  margin: 1rem 0;
`
