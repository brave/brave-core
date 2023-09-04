// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { WalletButton } from '../../../shared/style'


export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 32px 40px;
  border-radius: 16px;
  z-index: 2;
`

export const Header = styled.h1`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  color: ${leo.color.text.primary};
  margin: 0 0 8px 0;
  padding: 0;
  text-align: left;
  width: 100%;
`

export const Description = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
  margin: 0;
  padding: 0;
`

export const Link = styled.a`
  color: ${leo.color.button.background};
  text-decoration: none;
  font-weight: 600;
`

export const Underline = styled.span`
  text-decoration: underline;
`

export const ButtonRow = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  margin-top: 24px;
`

export const ConfirmButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 12px 16px;
  background-color: ${leo.color.button.background};
  border-radius: 1000px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.03em;
  color: ${leo.color.white};
  border: none;
  cursor: pointer;
`

export const CancelButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 12px 16px;
  background-color: transparent;
  border-radius: 1000px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.03em;
  color: ${leo.color.text.secondary};
  border: none;
  cursor: pointer;
`
