// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Styled Components
import {
  StyledWrapper,
  Header,
  Title,
  HeaderButton,
  CloseIcon,
  Modal,
  Divider,
  ModalContent,
} from './style'
import { Row } from '../../shared/style'

export interface Props {
  children?: React.ReactNode
  onClose: () => void
  onBack?: () => void
  title: string
  width?: string
  showDivider?: boolean
  hideHeader?: boolean
  headerPaddingVertical?: string
  headerPaddingHorizontal?: string
  height?: string
}

const ESC_KEY = 'Escape'

export const PopupModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      title,
      width,
      headerPaddingVertical,
      headerPaddingHorizontal,
      showDivider,
      hideHeader,
      onClose,
      onBack,
      height,
      children,
    } = props

    const handleKeyDown = React.useCallback(
      (event: KeyboardEvent) => {
        if (event.key === ESC_KEY) {
          onClose()
        }
      },
      [onClose],
    )

    React.useEffect(() => {
      document.addEventListener('keydown', handleKeyDown)

      return () => {
        document.removeEventListener('keydown', handleKeyDown)
      }
    }, [handleKeyDown])

    return (
      <StyledWrapper>
        <Modal
          width={width}
          height={height}
          ref={forwardedRef}
        >
          {!hideHeader && (
            <Header
              headerPaddingHorizontal={headerPaddingHorizontal}
              headerPaddingVertical={headerPaddingVertical}
            >
              <Row
                width='unset'
                gap='16px'
              >
                {onBack && (
                  <Button
                    onClick={onBack}
                    kind='outline'
                    size='small'
                    fab
                  >
                    <Icon name='arrow-left' />
                  </Button>
                )}
                <Title>{title}</Title>
              </Row>
              <HeaderButton onClick={onClose}>
                <CloseIcon />
              </HeaderButton>
            </Header>
          )}
          {showDivider && <Divider />}
          <ModalContent
            justifyContent='flex-start'
            width='100%'
            height='100%'
          >
            {children}
          </ModalContent>
        </Modal>
      </StyledWrapper>
    )
  },
)

export default PopupModal
