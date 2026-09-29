// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { Header } from '../header/header'

// Styles
import { CardSlot, DetailsHeader, DetailsPane } from '../../card_hub.style'
import { Column } from '$wallet/components/shared/style'

interface Props {
  title: string
  visible: boolean
  slotRef: React.Ref<HTMLDivElement>
  onBack?: () => void
}

export const CardDetails = (props: Props) => {
  const { title, visible, slotRef, onBack } = props

  return (
    <DetailsPane $visible={visible}>
      <DetailsHeader>
        <Header
          title={title}
          onBack={onBack}
        />
      </DetailsHeader>
      <Column
        width='100%'
        padding='0 16px 16px 16px'
      >
        <CardSlot ref={slotRef} />
      </Column>
    </DetailsPane>
  )
}
