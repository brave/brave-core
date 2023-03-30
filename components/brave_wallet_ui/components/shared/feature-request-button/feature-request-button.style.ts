// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import IdeaIcon from '../../../assets/svg-icons/idea.svg'

// Shared Styles
import { WalletButton } from '../style'

export const Button = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 8px 14px;
  position: fixed;
  bottom: 32px;
  right: 32px;
  background-color: ${p => p.theme.palette.blurple500};
  border: none;
  outline: none;
  border-radius: 40px;
  cursor: pointer;
  z-index: 10;
`

export const ButtonText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 21px;
  color: ${p => p.theme.palette.white};
`

export const IdeaButtonIcon = styled.span`
 width: 24px;
 height: 24px;
 background-color: ${(p) => p.theme.palette.white};
 -webkit-mask-image: url(${IdeaIcon});
 mask-image: url(${IdeaIcon});
 mask-size: cover;
 margin-right: 8px;
`
