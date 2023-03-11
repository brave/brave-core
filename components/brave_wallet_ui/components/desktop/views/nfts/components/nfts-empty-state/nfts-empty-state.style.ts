// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  margin-top: 36px;
`

export const EmptyStateImage = styled.img`
  width: 241px;
  height: auto;
  margin-bottom: 16px;
`

export const Heading = styled.h2`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  text-align: center;
  color: ${p => p.theme.color.text01};
  margin: 0 0 8px 0;
`

export const SubHeading = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  align-items: center;
  text-align: center;
  color: ${p => p.theme.color.text02};
  margin: 0 0 16px 0;
`

export const ImportButton = styled(WalletButton)`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 10px 22px;
  gap: 7px;
  background-color: ${p => p.theme.color.interactive05};
  border-radius: 48px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${p => p.theme.color.background01};
  margin-bottom: 64px;
  border: none;
  cursor: pointer;
`

export const DisclaimerText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  display: flex;
  align-items: center;
  text-align: center;
  color: ${p => p.theme.color.text03};
  margin: 0;
`
