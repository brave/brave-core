// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../../shared/style'
import GlobeConnectIcon from '../../../../assets/svg-icons/globe-connect-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  margin: 32px 78px 39px;
`

export const Title = styled.h4`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 18px;
  line-height: 26px;
  color: ${p => p.theme.color.text02};
  text-align: center;
  margin: 0 0 7px;
  align-self: center;
`

export const Description = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  color: ${p => p.theme.color.text02};
  margin: 0;
`

export const LearnMoreLink = styled.a`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  color: ${p => p.theme.color.interactive05};
  text-decoration: none;
  display: block;
  margin-top: 6px;
`

export const OpenRainbowAppButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 8px 14px;
  height: 40px;
  cursor: pointer;
  outline: none;
  border-radius: 40px;
  background-color: ${(p) => p.theme.palette.blurple500};
  border: none;
  align-self: center;
  margin-top: 36px;
  margin-bottom: 12px;
`

export const ButtonText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 21px;
  color: ${(p) => p.theme.palette.white};
`

export const GlobeIcon = styled.div`
  width: 24px;
  height: 24px;
  background: url(${GlobeConnectIcon});
  margin-right: 8px;
`

export const CheckboxWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  margin: 12px 0;
`
