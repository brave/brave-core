// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import SuccessSvg from '../../../../assets/svg-icons/success-circle-icon.svg'
import { TransactionStatusIcon, TransactionStatusText } from '../common/common.style'

export const SuccessIcon = styled(TransactionStatusIcon)`
  background: url(${SuccessSvg});
`

export const Title = styled(TransactionStatusText)`
  color: ${p => p.theme.color.text01};
`
