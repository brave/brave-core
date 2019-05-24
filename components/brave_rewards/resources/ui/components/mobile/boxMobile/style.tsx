/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from '../../../../theme'
import { Type } from './index'
import Card, { CardProps } from '../../../../components/layout/card'

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

export const StyledCard = styled(CustomCard)`
  margin-bottom: 12px;
`

export const StyledFlip = styled<{}, 'div'>('div')`
  display: block;
`

export const StyledContentWrapper = styled<StyleProps, 'div'>('div')`
  flex-direction: column;
  display: ${p => p.open ? 'flex' : 'none'};
`

export const StyledLeft = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  padding: 0 0 0 24px;
`

export const StyledRight = styled<{}, 'div'>('div')`
  display: flex;
  padding: 0 24px 0 0;
`

export const StyledTitle = styled<StyleProps, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  color: ${p => {
    if (p.enabled === false) return '#838391'
    return p.type && colors[p.type] || '#4b4c5c'
  }};
`

export const StyledBreak = styled<{}, 'div'>('div')`
  width: 100%;
  display: block;
`

export const StyledDescription = styled<StyleProps, 'div'>('div')`
  width: 100%;
  font-size: 14px;
  line-height: 1.7;
  padding: 16px 0;
  color: ${p => p.theme.color.text};
`

export const StyledContent = styled<StyleProps, 'div'>('div')`
  flex-basis: 100%;
  flex-grow: 1;
  text-align: ${p => p.contentShown ? 'default' : 'center'};
`

export const StyledSettingsWrapper = styled<StyleProps, 'div'>('div')`
  background: #fff;
  overflow: hidden;
  display: ${p => p.open ? 'block' : 'none'};
`

export const StyledSettingsClose = styled<{}, 'button'>('button')`
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

export const StyledSettingsTitle = styled<{}, 'span'>('span')`
  color: #4B4C5C;
  font-size: 16px;
  font-weight: 600;
  width: 100%;
  text-align: center;
`

export const StyledSettingsText = styled<{}, 'div'>('div')`
  color: #4B4C5C;
  font-size: 16px;
  font-weight: 600;
  display: flex;
  align-items: center;
`

export const StyleDetailsLink = styled<{}, 'a'>('a')`
  color: #4C54D2;
  font-size: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  font-weight: 600;
`

export const StyledDetailInfo = styled<{}, 'div'>('div')`
  width: 100%;
  padding: 0px 24px;
  display: block;
`

export const StyledDetailContent = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: column;
`

export const StyledChildContent = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  flex-direction: column;
  border-top: 1px solid #E5E5EA;
`

export const StyledSettingsContent = styled<{}, 'div'>('div')`
  width: 100%;
  padding: 24px;
  display: flex;
  flex-direction: column;
`

export const StyledSettingsHeader = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  padding: 0 24px;
`

export const StyledSettingsListTitle = styled<{}, 'span'>('span')`
  font-size: 16px;
  width: 100%;
  display: flex;
  justify-content: space-between;
`

export const StyledArrow = styled<{}, 'span'>('span')`
  color: #4C54D2;
  height: 16px;
  width: 16px;
  margin: 4px;
`

export const StyledToggleHeader = styled<StyleProps, 'div'>('div')`
  width: 100%;
  display: flex;
  align-items: center;
  ${p => getFixedStyling(p.detailView)}
  z-index: 1;
  justify-content: space-between;
  padding: 16px 0;
  border-bottom: 1px solid #E5E5EA;
`

export const StyledBackArrow = styled<{}, 'span'>('span')`
  height: 24px;
  width: 24px;
  margin: 0 8px 0 0;
`

export const StyledFullSizeWrapper = styled<{}, 'div'>('div')`
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

export const StyledSettingsIcon = styled<{}, 'button'>('button')`
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

export const StyledToggleWrapper = styled<StyleProps, 'div'>('div')`
  display: flex;
`

export const StyledMobileListWrapper = styled<{}, 'div'>('div')`
  padding: 0 24px;
`
