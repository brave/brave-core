// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import SubmittedOrSignedSvg from '../../../../assets/svg-icons/submitted-circle-icon.svg'
import { WalletButton } from '../../../shared/style'
import { TransactionStatusIcon, TransactionStatusText } from '../common/common.style'

export const SubmittedOrSignedIcon = styled(TransactionStatusIcon)`
  background: url(${SubmittedOrSignedSvg});
  background-size: contain;
`

export const Title = styled(TransactionStatusText)`
  color: ${p => p.theme.color.text01};
`

export const DetailButton = styled(WalletButton)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 20px;
  text-align: center;
  color: ${p => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0;
  padding: 0;
`
