/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Button, { Props as ButtonProps } from '../../../components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 20px;
  font-weight: 500;
  color: #4b4c5c;
  margin-bottom: 48px;
`

export const StyledNote = styled<{}, 'div'>('div') `
  max-width: 508px;
  font-family: Muli,sans-serif;
  font-size: 12px;
  font-weight: 300;
  line-height: 1.5;
  color: #686978;
`

export const StyledAddresses = styled<{}, 'div'>('div') `
  display: flex;
  flex-wrap: wrap;
  margin: 0 -15px;
  align-items: stretch;
`

export const StyledAddress = styled<{}, 'div'>('div') `
  flex-basis: 50%;
  flex-shrink: 0;
  flex-grow: 0;
  width: 50%;
  box-sizing: border-box;
  padding: 0 15px 26px;
`

export const StyledLogo = styled<{}, 'div'>('div') `
  height: 60px;
  flex-basis: 60px;
  flex-shrink: 0;
  margin-right: 20px;
`

export const StyledData = styled<{}, 'div'>('div') `
  flex-basis: 100%;
  text-align: center;
  margin-top: 22px;
  color: #686978;
`

export const StyledAddressTitle = styled<{}, 'div'>('div') `
  flex-basis: 30%;
  flex-grow: 1;
  font-size: 16px;
  line-height: 1.38;
  color: #4b4c5c;
`

export const StyledShowQR = styled<{}, 'div'>('div') `
  width: 110px;
  height: 110px;
  justify-content: center;
  display: flex;
  background: #eee;
`

export const StyledQRImageWrapper = styled<{}, 'div'>('div') `
  flex-basis: 100%;
  justify-content: center;
  display: flex;
  margin: 15px 0 10px;
  position: relative;
`

export const StyledQRImage = styled<{}, 'img'>('img') `
  width: 110px;
  height: 110px;
`

export const StyledQRButton = styled(Button as ComponentType<ButtonProps>) `
  position: absolute;
  top: calc(50% - 23px);
  font-weight: 400;
`

export const StyledLink = styled<{}, 'a'>('a') `
  color: #4c54d2;
  text-decoration: none;

  &:hover {
    text-decoration: underline;
  }
`

export const StyledHeader = styled<{}, 'div'>('div') `
  display: flex;
  align-items: center;
  flex-wrap: wrap;
`

export const StyledWalletAddress = styled<{}, 'div'>('div') `
  font-size: 12px;
  font-weight: 500;
  line-height: 1;
  color: #4b4c5c;
  text-align: left;
  margin-bottom: 4px;
`
