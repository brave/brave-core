/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  amount: number
}

export function TokenAmount (props: Props) {
  return (
    <>
      <span className='amount'>{props.amount.toFixed(3)}</span>&nbsp;
      <span className='currency'>BAT</span>
    </>
  )
}
