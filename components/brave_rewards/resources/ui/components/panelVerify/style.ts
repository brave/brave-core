/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Heading from '../../../components/text/heading'
import Button, { Props as ButtonProps } from '../../../components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const StyledWrapper = styled<{}, 'div'>('div')`
  position: absolute;
  text-align: center;
  font-family: Poppins, sans-serif;
  background-image: linear-gradient(180deg, #4C54D2 0%, #563195 100%);
  padding: 42px;
  top: 0;
  left: 0;
  width: 100%;
  z-index: 10;
  height: 100%;
  overflow-y: auto;
`

export const StyledClose = styled<{}, 'div'>('div')`
  position: absolute;
  top: 20px;
  right: 20px;
  cursor: pointer;
  width: 11px;
  height: 11px;
  color: #fff;
`

export const StyledHeader = styled<{}, 'div'>('div')`
  margin-bottom: 42px;
`

export const StyledBatIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: middle;
  width: 50px;
  height: 50px;
`

export const StyledHeaderText = styled<{}, 'div'>('div')`
  width: calc(100% - 50px);
  vertical-align: middle;
  display: inline-block;
  padding-left: 22px;
`

export const StyledTitle = styled(Heading)`
  color: #fff;
  text-align: left;
  font-size: 22px;
  line-height: 24px;
  font-weight: 600;
`

export const StyledSubtitle = styled(Heading)`
  color: #fff;
  text-align: left;
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 24px;
`

export const StyledListTitle = styled(Heading)`
  font-weight: 600;
  font-size: 14px;
  text-align: left;
  color: #fff;
`

export const StyledListItem = styled<{}, 'div'>('div')`
  color: #fff;
  margin: 15px 0;
`

export const StyledListIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 24px;
  height: 24px;
  vertical-align: middle;
`

export const StyledListItemText = styled<{}, 'div'>('div')`
  vertical-align: middle;
  text-align: left;
  font-family: Muli, sans-serif;
  font-size: 15px;
  line-height: 20px;
  display: inline-block;
  width: calc(100% - 30px);
  padding-left: 12px;
`

export const StyledIDNotice = styled<{}, 'div'>('div')`
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  color: #fff;
  padding: 20px 24px;
  border-top: 1px solid rgba(255, 255, 255, 0.5433);
  margin: 30px 0 0;
`

export const StyledButton = styled(Button as ComponentType<ButtonProps>)`
  padding: 11px 15px;
`

export const StyledFooter = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  color: #fff;
  margin-top: 25px;
`

export const StyledFooterIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: middle;
  width: 20px;
  height: 20px;
`
