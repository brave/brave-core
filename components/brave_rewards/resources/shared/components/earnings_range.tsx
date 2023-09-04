/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TokenAmount } from './token_amount'

interface Props {
  minimum: number
  maximum: number
  minimumFractionDigits?: number
}

export function EarningsRange (props: Props) {
  if (props.maximum <= 0) {
    return <TokenAmount amount={0} minimumFractionDigits={3} />
  }
  return (
    <>
      <TokenAmount
        amount={props.minimum}
        minimumFractionDigits={3}
        hideCurrency
      />{' - '}
      <TokenAmount amount={props.maximum} minimumFractionDigits={3} />
    </>
  )
}
