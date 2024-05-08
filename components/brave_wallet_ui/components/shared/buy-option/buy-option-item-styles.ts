/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { LoaderIcon } from 'brave-ui/components/icons'

import { WalletButton } from '../style'

interface LayoutProps {
  layoutType?: 'loose' | 'tight'
}

export const StyledWrapper = styled.div<LayoutProps>`
  display: flex;
  flex-direction: row;
  padding: 8px 16px;
  align-items: center;
  justify-content: space-between;

  ${(p) =>
    p?.layoutType === 'loose'
      ? css`
          width: 100%;
          margin-top: 16px;
          margin-bottom: 4px;
          border: 1px solid ${(p) => p.theme.color.divider01};
          border-radius: 4px;
          padding: 12px;
        `
      : ''}
`

export const Logo = styled.img`
  width: 40px;
  height: auto;
  margin-right: 16px;
`

export const Content = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
`

export const Name = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 16px;
  line-height: 28px;
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const Description = styled.span`
  font-family: 'Poppins';
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
`

export const StyledButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 1000px;
  padding: 9px 12px;
  outline: none;
  background-color: transparent;
  border: 1px solid ${leo.color.divider.strong};
  white-space: nowrap;
`

export const ButtonText = styled.span`
  font-family: 'Poppins';
  font-size: 12px;
  font-weight: 600;
  line-height: 16px;
  letter-spacing: 0.36px;
  color: ${leo.color.text.interactive};
  text-align: center;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${leo.color.text.interactive};
  height: 13px;
  width: 13px;
`
