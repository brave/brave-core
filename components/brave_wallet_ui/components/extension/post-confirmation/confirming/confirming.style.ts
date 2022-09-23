// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import ConfirmingSvg from '../../../../assets/svg-icons/confirming-circle-icon.svg'
import { TransactionStatusIcon, TransactionStatusText } from '../common/common.style'

export const ConfirmingIcon = styled(TransactionStatusIcon)`
  background: url(${ConfirmingSvg});
`

export const Title = styled(TransactionStatusText)`
  color: ${p => p.theme.color.text01};
`

export const ConfirmingText = styled.div`
  position: relative;
  top: 40px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 10px;
  line-height: 15px;

  text-align: center;
  letter-spacing: 0.02em;

  color: ${p => p.theme.color.text02};
`

export const ConfirmationsNumber = styled.div`
  position: relative;
  top: 42px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 21px;

  text-align: center;
  letter-spacing: 0.02em;

  color: ${p => p.theme.color.text01};
`
