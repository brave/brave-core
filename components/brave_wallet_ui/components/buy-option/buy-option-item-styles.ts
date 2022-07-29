/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { WalletButton } from '../shared/style'
import { LoaderIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  padding: 16px 0;
  align-items: flex-start;
`

export const Logo = styled.img`
  width: 46px;
  height: auto;
  margin-right: 20px;
  margin-top: 16px;
`

export const Content = styled.div`
  display: flex;
  flex-direction: column;
`

export const Name = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 18px;
  line-height: 26px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${p => p.theme.color.text01};
  padding-bottom: 2px;
`

export const Description = styled.span`
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text01};
  margin-bottom: 12px;
`

export const StyledButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 44px;
  padding: 10px 20px;
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
