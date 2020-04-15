/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { BatColorIcon } from 'brave-ui/components/icons'

import backgroundSrc from './assets/dialog_bg.svg'

export const MainPanel = styled.div<{ showBackground?: boolean }>`
  position: relative;
  top: 0;
  left: 0;
  max-width: 548px;
  background: ${p => p.theme.color.panelBackground};
  font-weight: normal;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  color: ${p => p.theme.palette.black};

  ${p => !p.showBackground ? '' : css`
    background-image: url(${backgroundSrc});
    background-repeat: no-repeat;
    background-position: bottom right;
  `}

  a {
    color: ${p => p.theme.color.brandBat};
    text-decoration: none;
  }
`

export const TopBar = styled.div`
  padding: 15px 15px 8px;
  display: flex;
`

export const TitleContainer = styled.div`
  color: ${p => p.theme.palette.grey600};
  font-weight: 500;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 12px;
  text-transform: uppercase;
  flex-grow: 1;
`

export const DialogTitleIcon = styled(BatColorIcon)`
  display: inline-block;
  vertical-align: middle;
  position: relative;
  top: -1px;
  left: 0;
  height: 17px;
  width: auto;
  padding-right: 4px;
`

export const BatText = styled.span`
  color: ${p => p.theme.palette.neutral700};
`

export const CloseButton = styled.button`
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
  width: 14px;
  height: 14px;
  color: ${p => p.theme.palette.neutral600};
  margin: 7px 6px 0 0;
`

export const Content = styled.div`
  padding: 8px 49px 34px;
`
