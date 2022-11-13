/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

import { WalletButton } from '../style'

interface LayoutProps {
  layoutType?: 'loose' | 'tight'
}

export const StyledWrapper = styled.div<LayoutProps>`
  display: flex;
  flex-direction: row;
  padding: 10px 0;
  align-items: flex-start;
  
  ${(p) => p?.layoutType === 'loose'
    ? css`
      width: 100%;
      margin-top: 16px;
      margin-bottom: 4px;
      border: 1px solid ${(p) => p.theme.color.divider01};
      border-radius: 4px;
      padding: 12px;
    `
    : ''
  }
`

export const Logo = styled.img`
  width: 46px;
  height: auto;
  margin-right: 16px;
  margin-top: 16px;
`

export const Content = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
`

export const Name = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 18px;
  line-height: 26px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${p => p.theme.color.text01};
`

export const Description = styled.span`
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color.text02};
  margin-bottom: 4px;
`

export const StyledButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 44px;
  padding: 6px 18px;
  outline: none;
  background-color: transparent;
  border: ${p => `1px solid ${p.theme.color.interactive08}`};
  
`

export const ButtonText = styled.span`
  font-family: 'Poppins';
  font-size: 13px;
  font-weight: 600;
  color: ${p => p.theme.color.interactive07};
  text-align: center;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 13px;
  width: 13px;
`
