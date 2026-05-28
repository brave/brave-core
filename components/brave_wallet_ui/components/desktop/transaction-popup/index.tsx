// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Shared Styles
import { Text } from '../../shared/style'

// Styled Components
import { StyledWrapper, PopupButton, ClickAwayContainer } from './style'

interface TransactionPopupItemProps {
  onClick: () => void
  text: string
}

export const TransactionPopupItem = (props: TransactionPopupItemProps) => (
  <PopupButton onClick={props.onClick}>
    <Text
      textAlign='left'
      variant='default.semibold'
      textColor='primary'
    >
      {props.text}
    </Text>
  </PopupButton>
)

interface Props {
  children?: React.ReactNode
}

const TransactionPopup = (props: Props) => {
  return (
    <>
      <StyledWrapper>{props.children}</StyledWrapper>
      <ClickAwayContainer />
    </>
  )
}

export default TransactionPopup
