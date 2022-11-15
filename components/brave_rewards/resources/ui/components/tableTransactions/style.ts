/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { SummaryType as TransactionType } from '../modalActivity'

const colors: Record<TransactionType, string> = {
  grant: '#9f22a1',
  ads: '#9752cb',
  contribute: '#696fdc',
  monthly: '#696fdc',
  tip: '#696fdc'
}

const getColor = (p: StyleProps) => {
  const color = colors[p.type]

  return css`
    --tableTransactions-type-color: ${color};
  `
}

interface StyleProps {
  type: TransactionType
}

export const StyledTHLast = styled('div')<{}>`
  text-align: right;
  padding-right: 14px;
`

export const StyledProvider = styled('span')<{}>`
  color: #9e9fab;
`

export const StyledType = styled('div')<StyleProps>`
  ${getColor};
  color: var(--tableTransactions-type-color);
`
