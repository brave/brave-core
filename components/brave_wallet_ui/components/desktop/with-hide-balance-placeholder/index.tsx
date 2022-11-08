// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import {
  PlaceholderText
} from './style'

export interface Props {
  children?: React.ReactNode
  hideBalances: boolean
  size: 'big' | 'small'
}

function WithHideBalancePlaceholder (props: Props) {
  const {
    children,
    hideBalances,
    size
  } = props

  return (
    <>
      {hideBalances ? (
        <PlaceholderText isBig={size === 'big'} >******</PlaceholderText>
      ) : (
        children
      )}
    </>
  )
}

export default WithHideBalancePlaceholder
