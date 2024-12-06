/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Optional } from '../../shared/lib/optional'
import { TextSkeleton } from './text_skeleton'

const balanceFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 4
})

interface Props {
  balance: Optional<number>
}

export function AccountBalance(props: Props) {
  if (!props.balance.hasValue()) {
    return <TextSkeleton length={12} />
  }
  return <>
    {balanceFormatter.format(props.balance.valueOr(0)) + ' BAT'}
  </>
}
