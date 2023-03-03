// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Assets
import WarningIcon from '../../assets/warning-icon.svg'

// Types
import { AddressMessageInfo } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { LearnMoreLink, HowToSolveButton, ErrorIcon } from './address-message.style'
import { Column, Row, Text, VerticalDivider, VerticalSpacer } from '../../shared.styles'

interface Props {
  addressMessageInfo: AddressMessageInfo
  onClickHowToSolve?: () => void
}

export const AddressMessage = (props: Props) => {
  const { addressMessageInfo, onClickHowToSolve } = props

  return (
    <Column
      columnWidth='full'
      horizontalPadding={16}
      horizontalAlign='flex-start'
    >
      <VerticalDivider marginBottom={16} />
      <Row>
        {addressMessageInfo.type &&
          <ErrorIcon icon={WarningIcon} size={15} type={addressMessageInfo.type} />
        }
        <Text
          textSize='14px'
          textColor='text01'
          isBold={true}
          textAlign='left'
        >
          {getLocale(addressMessageInfo.title)}
        </Text>
      </Row>
      {
        addressMessageInfo.description &&
        <>
          <VerticalSpacer size={8} />
          <Row
            rowWidth='full'
          >
            <Text
              textSize='12px'
              textColor='text03'
              isBold={false}
              textAlign='left'
            >
              {getLocale(addressMessageInfo.description)} {addressMessageInfo.url &&
                <LearnMoreLink
                  href={addressMessageInfo.url}
                  target='_blank'
                  rel='noopener noreferrer'>
                  {getLocale('braveWalletLearnMore')}
                </LearnMoreLink>
              }
              {onClickHowToSolve &&
                <HowToSolveButton onClick={onClickHowToSolve}>
                  {getLocale('braveWalletHowToSolve')}
                </HowToSolveButton>
              }
            </Text>
          </Row>
        </>
      }
      <VerticalSpacer size={16} />
    </Column >
  )
}

export default AddressMessage
