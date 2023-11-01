// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import { StandardModal } from '../../modals/standard-modal/standard-modal'

// Styled Components
import { Link, Section } from './privacy-modal.style'
import {
  Row,
  Column,
  Text,
  IconButton,
  VerticalSpacer,
  Icon
} from '../../shared-swap.styles'

interface Props {
  onClose: () => void
}

export const PrivacyModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { onClose } = props

    return (
      <StandardModal
        ref={forwardedRef}
        modalHeight='dynamic'
        modalBackground='background02'
      >
        <Row
          rowWidth='full'
          horizontalPadding={24}
          verticalPadding={20}
        >
          <Text
            textSize='18px'
            isBold={true}
          >
            {getLocale('braveSwapPrivacyPolicy')}
          </Text>
          <IconButton onClick={onClose}>
            <Icon
              size={24}
              name='close'
            />
          </IconButton>
        </Row>
        <Column
          columnWidth='full'
          columnHeight='full'
          horizontalPadding={20}
          horizontalAlign='flex-start'
        >
          <Text
            textSize='16px'
            textColor='text02'
            textAlign='left'
            isBold={true}
          >
            {getLocale('braveSwapPrivacyDescription')}
          </Text>
          <VerticalSpacer size={10} />
          <Section
            columnWidth='full'
            horizontalAlign='flex-start'
            verticalPadding={10}
            horizontalPadding={10}
          >
            <Link
              rel='noopener noreferrer'
              target='_blank'
              href='https://www.0x.org/'
            >
              0x
            </Link>
            <Text
              textSize='14px'
              textColor='text03'
              textAlign='left'
              isBold={true}
            >
              {getLocale('braveSwapV2Disclaimer')
                .replaceAll('$1', '0x')
                .replace('$2', 'EVM')
                .replace('$3', 'Ethereum')}
            </Text>
            <Link
              rel='noopener noreferrer'
              target='_blank'
              href='https://www.0x.org/privacy'
            >
              {getLocale('braveSwapV2Privacy').replace('$1', '0x')}
            </Link>
          </Section>
          <VerticalSpacer size={20} />
          <Section
            columnWidth='full'
            horizontalAlign='flex-start'
            verticalPadding={10}
            horizontalPadding={10}
          >
            <Link
              rel='noopener noreferrer'
              target='_blank'
              href='https://jup.ag/'
            >
              Jupiter
            </Link>
            <Text
              textSize='14px'
              textColor='text03'
              textAlign='left'
              isBold={true}
            >
              {getLocale('braveSwapV2Disclaimer')
                .replaceAll('$1', 'Jupiter')
                .replace('$2', 'Solana')
                .replace('$3', 'Solana')}
            </Text>
            <Link
              rel='noopener noreferrer'
              target='_blank'
              href='https://docs.jup.ag/legal/privacy-policy'
            >
              {getLocale('braveSwapV2Privacy').replace('$1', 'Jupiter')}
            </Link>
          </Section>
          <VerticalSpacer size={20} />
        </Column>
      </StandardModal>
    )
  }
)
