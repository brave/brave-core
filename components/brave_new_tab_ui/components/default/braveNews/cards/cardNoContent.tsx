// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'
import { UnidealMessageCard } from './cardError'

interface Props {
  onCustomize: () => unknown
}

export default function CardNoContent (props: Props) {
  return (
    <UnidealMessageCard
      heading={getLocale('braveNewsNoContentHeading')}
      message={getLocale('braveNewsNoContentMessage')}
      actionLabel={getLocale('braveNewsNoContentActionLabel')}
      onActionClick={props.onCustomize}
    />
  )
}
