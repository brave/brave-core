// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  Header,
  Title,
  CloseButton,
  Modal,
  Divider
} from './style'

export interface Props {
  children?: React.ReactNode
  onClose: () => void
  title: string
  width?: string
  showDivider?: boolean
  hideHeader?: boolean
  headerPaddingVertical?: number
  headerPaddingHorizontal?: number
  borderRadius?: number
}

const ESC_KEY = 'Escape'

const PopupModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      title,
      width,
      borderRadius,
      headerPaddingVertical,
      headerPaddingHorizontal,
      showDivider,
      hideHeader,
      onClose,
      children
    } = props

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === ESC_KEY) {
        onClose()
      }
    }

    React.useEffect(() => {
      document.addEventListener('keydown', handleKeyDown)

      return () => {
        document.removeEventListener('keydown', handleKeyDown)
      }
    }, [])

    return (
      <StyledWrapper>
        <Modal width={width} borderRadius={borderRadius} ref={forwardedRef}>
          {!hideHeader &&
            <Header
              headerPaddingHorizontal={headerPaddingHorizontal}
              headerPaddingVertical={headerPaddingVertical}
            >
              <Title>{title}</Title>
              <CloseButton onClick={onClose} />
            </Header>
          }
          {showDivider && <Divider />}
          {children}
        </Modal>
      </StyledWrapper>
    )
  }
)

export default PopupModal
