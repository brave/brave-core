/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from '../../../theme'
import { Type } from './index'
import Card, { CardProps } from '../../../components/layout/card'

interface StyleProps {
  open?: boolean
  float?: string
  checked?: boolean
  type?: Type
}

const colors: Record<Type, string> = {
  ads: '#C12D7C',
  contribute: '#9F22A1',
  donation: '#696FDC'
}

export const StyledWrapper = styled<StyleProps, 'div'>('div')`
  width: 100%;
  margin: 0 0 24px;
`

const CustomCard: React.FC<CardProps> = (props) =>
  <Card emphasis={'60'} {...props} />

export const StyledCard = styled(CustomCard)`
  font-size: 14px;
`

export const StyledFlip = styled<StyleProps, 'div'>('div')`
  display: flex;
  align-items: center;
  justify-content: center;
`

export const StyledContentWrapper = styled<StyleProps, 'div'>('div')`
  display: ${p => p.open ? 'flex' : 'none'};
  flex-direction: column;
`

export const StyledTitle = styled<StyleProps, 'div'>('div')`
  display: flex;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 20px;
  font-weight: 600;
  color: ${p => p.type && colors[p.type] || '#4b4c5c'};
`

export const StyledDescription = styled<{}, 'div'>('div')`
  width: 100%;
  font-size: 15px;
  color: ${p => p.theme.color.text};
  padding: 12px 0 0;
  line-height: 1.7;
`

export const StyledSettingsIcon = styled<StyleProps, 'button'>('button')`
  width: 24px;
  height: 24px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
  color: #A1A8F2;
`

export const StyledContent = styled<{}, 'div'>('div')`
  flex-basis: 100%;
  flex-grow: 1;
  margin-top: 25px;
`

export const StyledSettingsWrapper = styled<StyleProps, 'div'>('div')`
  background: #fff;
  display: ${p => p.open ? 'block' : 'none'};
  width: 100%;
`

export const StyledSettingsClose = styled<StyleProps, 'button'>('button')`
  display: ${p => p.open ? 'block' : 'none'};
  position: absolute;
  right: 24px;
  top: 24px;
  width: 24px;
  height: 24px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
  color: ${p => p.theme.palette.grey600};
`
export const StyledSettingTitleWrapper = styled('div')`
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 100%;
`

export const StyledSettingsTitle = styled<{}, 'div'>('div')`
  display: flex;
  margin: 0 0 24px;
`

export const StyledSettingsToggleContainer = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
`

export const StyledSettingsText = styled<{}, 'div'>('div')`
  font-size: 20px;
  font-weight: 600;
  font-family: ${p => p.theme.fontFamily.heading};
  color: ${p => p.theme.color.text};
  display: flex;
  align-items: center;
`
