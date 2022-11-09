// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import ErrorSvg from '../../../../assets/svg-icons/error-circle-icon.svg'
import { TransactionStatusIcon, TransactionStatusText } from '../common/common.style'

export const ErrorIcon = styled(TransactionStatusIcon)`
  background: url(${ErrorSvg});
`

export const Title = styled(TransactionStatusText)`
  color: ${p => p.theme.color.errorBorder};
`

export const ErrorDetailTitle = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color.errorBorder};
  margin: 0 16px;
`

export const ErrorDetailContentContainer = styled.div`
  background: ${p => p.theme.color.errorBackground};
  border-radius: 8px;
  margin: 16px;
`

export const ErrorDetailContent = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color.text01};
  opacity: 0.9;
  margin: 8px;
`
