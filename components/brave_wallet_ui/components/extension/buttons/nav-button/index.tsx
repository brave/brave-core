// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  StyledButton,
  ButtonText,
  RejectIcon,
  SignIcon,
  ConfirmIcon,
  StyledLink
} from './style'

export type PanelButtonTypes =
  | 'primary'
  | 'secondary'
  | 'danger'
  | 'confirm'
  | 'sign'
  | 'reject'
  | 'cancel'

interface BaseProps {
  buttonType: PanelButtonTypes
  text: string | undefined
  disabled?: boolean
  needsTopMargin?: boolean
  maxHeight?: string
  minHeight?: string
  minWidth?: string
}

type ClickProps = (
  {
    onSubmit: () => void
    url?: string
  }
  | {
    url: string
    onSubmit?: () => void
  }
)

export type Props = BaseProps & ClickProps

export const NavButton: React.FC<Props> = ({
  buttonType,
  disabled,
  maxHeight,
  minHeight,
  minWidth,
  needsTopMargin,
  onSubmit,
  text,
  url
}) => {
  // memos
  const buttonContent = React.useMemo(() => {
    return <>
      {buttonType === 'reject' &&
        <RejectIcon />
      }
      {buttonType === 'sign' &&
        <SignIcon />
      }
      {buttonType === 'confirm' &&
        <ConfirmIcon />
      }
      <ButtonText buttonType={buttonType}>{text}</ButtonText>
    </>
  }, [buttonType, text])

  // render
  return url ? (
    <StyledLink
      disabled={disabled}
      buttonType={buttonType}
      onClick={onSubmit}
      to={url || ''}
      maxHeight={maxHeight}
      minWidth={minWidth}
    >
      {buttonContent}
    </StyledLink>
  ) : (
    <StyledButton
      disabled={disabled}
      buttonType={buttonType}
      onClick={onSubmit}
      addTopMargin={needsTopMargin && text ? text.length > 20 : false}
      maxHeight={maxHeight}
      minWidth={minWidth}
      minHeight={minHeight}
    >
      {buttonContent}
    </StyledButton>
  )
}

export default NavButton
