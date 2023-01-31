// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { AlertType } from '../../../constants/types'

// style
import {
  Column,
  Text,
  WarningCircleFilledIcon,
  WarningTriangleFilledIcon
} from '../../shared/style'
import { InlineAlertContainer } from './alert-inline.style'

interface Props {
  alertType: AlertType
  message: string
  onDismiss?: () => void
  onPrimaryClick?: () => void
  onSecondaryClick?: () => void
  primaryButtonText?: string
  secondaryButtonText?: string
  title?: string
}

const ICON_SIZE = { width: 16, height: 16 }

export const AlertInline: React.FC<Props> = ({
  alertType,
  message,
  onDismiss,
  onPrimaryClick,
  onSecondaryClick,
  primaryButtonText,
  secondaryButtonText,
  title
}) => {
  return (
    <InlineAlertContainer alertType={alertType}>
      <Column>
        {alertType === 'danger' && (
          <WarningCircleFilledIcon style={ICON_SIZE} />
        )}
        {alertType === 'warning' && (
          <WarningTriangleFilledIcon style={ICON_SIZE} />
        )}
      </Column>
      <Column alignItems='center' justifyContent='center' fullHeight>
        <Text textAlign='left' textSize='14px'>
          {message}
        </Text>
      </Column>
    </InlineAlertContainer>
  )
}

export default AlertInline
