// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../../../../constants/types'

// selectors
import { useSafeUISelector } from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// components
import PopupModal from '../../../../popup-modals'

// styles
import * as leo from '@brave/leo/tokens/css'
import {
  Column,
  Row,
  ScrollableColumn,
  VerticalSpace
} from '../../../../../shared/style'
import { Category, Name, Description } from './dapp-details-modal.styles'


interface Props {
  dapp: BraveWallet.Dapp
  onClose: () => void
}

export const DappDetailsModal = ({ dapp, onClose }: Props) => {
  const { name, categories, description } = dapp

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return (
    <PopupModal
      onClose={onClose}
      title='Details'
      width='500px'
      borderRadius={16}
    >
      <ScrollableColumn>
        <Column justifyContent='center' margin={isPanel ? `0 ${leo.spacing.xl}` : `0 ${leo.spacing['2Xl']}`}>
          <div
            style={{
              display: 'flex',
              flexShrink: 0,
              width: '72px',
              height: '72px',
              backgroundColor: 'blue',
              borderRadius: '50%',
              paddingTop: '48px'
            }}
          />
          <VerticalSpace space='8px' />
          <Name>{name}</Name>
          <VerticalSpace space='8px' />
          <Row gap='8px'>
            {categories.map((category) => (
              <Category key={category}>{category}</Category>
            ))}
          </Row>
          <VerticalSpace space='8px' />
          <Description>{description}</Description>
        </Column>
      </ScrollableColumn>
    </PopupModal>
  )
}
