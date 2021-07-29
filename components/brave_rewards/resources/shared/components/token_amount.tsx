/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  amount: number
  minimumFractionDigits?: number
}

const defaultMinimumFractionDigits = 3

function getFormatter (props: Props) {
  return new Intl.NumberFormat(undefined, {
    maximumFractionDigits: 3,
    minimumFractionDigits: typeof props.minimumFractionDigits === 'number'
      ? props.minimumFractionDigits
      : defaultMinimumFractionDigits
  })
}

export function TokenAmount (props: Props) {
  const formatter = getFormatter(props)
  return (
    <>
      <span className='amount'>{formatter.format(props.amount)}</span>&nbsp;
      <span className='currency'>BAT</span>
    </>
  )
}
