// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../components/shared/style'
import IdeaIcon from '../../assets/svg-icons/idea.svg'

export const WalletWidgetStandIn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 285px;
  min-width: 285px;
  @media screen and (max-width: 800px) {
    margin-bottom: 40px;
  }
`

export const SimplePageWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const FeatureRequestButton = styled(WalletButton)`
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
