/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'brave-ui/theme'
import { Type } from './index'
import Card, { CardProps } from 'brave-ui/components/layout/card'
import { AlertCircleIcon } from 'brave-ui/components/icons'

interface StyleProps {
  open?: boolean
  float?: string
  type?: Type
  enabled?: boolean
  detailView?: boolean
  contentShown?: boolean
}

const colors: Record<Type, string> = {
  ads: '#C12D7C',
  contribute: '#9F22A1',
  donation: '#696FDC'
}

const getFixedStyling = (detailView?: boolean) => {
  if (!detailView) {
    return null
  }

  return css`
    top: 0px;
    left: 0px;
    position: fixed;
    background: #fff;
  `
}

const CustomCard: React.FC<CardProps> = (props) =>
  <Card useDefaultContentPadding={false} {...props} />

export const BoxCard = styled(CustomCard)`
  margin-bottom: 12px;
`

export const Flip = styled<{}, 'div'>('div')`
  display: block;
`

export const ContentWrapper = styled<StyleProps, 'div'>('div')`
  flex-direction: column;
  display: ${p => p.open ? 'flex' : 'none'};
`

export const Left = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  padding: 0 0 0 24px;
`

export const Right = styled<{}, 'div'>('div')`
  display: flex;
  padding: 0 24px 0 0;
`

export const Title = styled<StyleProps, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  color: ${p => {
    if (p.enabled === false) return '#838391'
    return p.type && colors[p.type] || '#4b4c5c'
  }};
`

export const Break = styled<{}, 'div'>('div')`
  width: 100%;
  display: block;
`

export const Description = styled<StyleProps, 'div'>('div')`
  width: 100%;
  font-size: 14px;
  line-height: 1.7;
  padding: 16px 0;
  color: ${p => p.theme.color.text};
`

export const Content = styled<StyleProps, 'div'>('div')`
  flex-basis: 100%;
  flex-grow: 1;
  text-align: ${p => p.contentShown ? 'default' : 'center'};
`

export const SettingsWrapper = styled<StyleProps, 'div'>('div')`
  background: #fff;
  overflow: hidden;
  display: ${p => p.open ? 'block' : 'none'};
`

export const SettingsClose = styled<{}, 'button'>('button')`
  display: block;
  position: absolute;
  right: 16px;
  top: 16px;
  width: 20px;
  height: 20px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
  color: ${p => p.theme.palette.grey600};
`

export const SettingsTitle = styled<{}, 'span'>('span')`
  color: #4B4C5C;
  font-size: 16px;
  font-weight: 600;
  width: 100%;
  text-align: center;
`

export const SettingsText = styled<{}, 'div'>('div')`
  color: #4B4C5C;
  font-size: 16px;
  font-weight: 600;
  display: flex;
  align-items: center;
`

export const DetailsLink = styled<{}, 'a'>('a')`
  color: #4C54D2;
  font-size: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  font-weight: 600;
`

export const DetailInfo = styled<{}, 'div'>('div')`
  width: 100%;
  padding: 0px 24px;
  display: block;
`

export const DetailContent = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: column;
`

export const ChildContent = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  flex-direction: column;
  border-top: 1px solid #E5E5EA;
`

export const SettingsContent = styled<{}, 'div'>('div')`
  width: 100%;
  padding: 24px;
  display: flex;
  flex-direction: column;
`

export const SettingsHeader = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  padding: 0 24px;
`

export const SettingsListTitle = styled<{}, 'span'>('span')`
  font-size: 16px;
  width: 100%;
  display: flex;
  justify-content: space-between;
`

export const Arrow = styled<{}, 'span'>('span')`
  color: #4C54D2;
  height: 16px;
  width: 16px;
  margin: 4px;
`

export const ToggleHeader = styled<StyleProps, 'div'>('div')`
  width: 100%;
  display: flex;
  align-items: center;
  ${p => getFixedStyling(p.detailView)}
  z-index: 1;
  justify-content: space-between;
  padding: 16px 0;
  border-bottom: 1px solid #E5E5EA;
`

export const BackArrow = styled<{}, 'span'>('span')`
  height: 24px;
  width: 24px;
  margin: 0 8px 0 0;
`

export const FullSizeWrapper = styled<{}, 'div'>('div')`
  display: block;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: #fff;
  z-index: 999;
  overflow-y: scroll;
  padding: 64px 0 0;
`

export const SettingsIcon = styled<{}, 'button'>('button')`
  width: 24px;
  height: 24px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
  color: #A1A8F2;
  position: absolute;
  right: 24px;
  top: 16px;
`

export const ToggleWrapper = styled<StyleProps, 'div'>('div')`
  display: flex;
`

export const MobileListWrapper = styled<{}, 'div'>('div')`
  padding: 0 24px;
`

export const Alert = styled<{}, 'div'>('div')`
  background: ${p => p.theme.color.infoBackground};
  padding: 18px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
`

// TODO: use a React FC which can:
// - decide the correct icon to use based on alert props
// - decide the correct color to use based on theme and props
// But we can't use that until we can get a ThemeContext or useTheme hook
// from styled-components 4.x
export const AlertIcon = styled(AlertCircleIcon)`
  align-self: flex-start;
  flex-shrink: 0;
  height: 24px;
  width: 24px;
  margin-right: 8px;
  color: ${p => p.theme.color.infoForeground};
`

export const AlertContent = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  flex-direction: row;
  font-size: 14px;
  min-height: 100%;
`
