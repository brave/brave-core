// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../../../../constants/types'

// utils
import { getLocale } from '../../../../../../../common/locale'

// components
import { Dapp } from '../dapp/dapp'

// styles
import {
  CategoryTitle,
  CategoryWrapper,
  DappGrid,
  ListToggleButton
} from './dapp-category.styles'

export const DappCategory = ({
  category,
  dapps,
  expanded,
  onShowMore
}: {
  category: string
  dapps: BraveWallet.Dapp[]
  expanded: boolean
  onShowMore: () => void
}) => {
  return (
    <CategoryWrapper>
      <CategoryTitle>{category}</CategoryTitle>
      <DappGrid>
        {dapps.map((dapp) => (
          <Dapp key={`${category}-${dapp.id}`} dapp={dapp} />
        ))}
      </DappGrid>
      <ListToggleButton onClick={onShowMore}>
        {getLocale(
          expanded
            ? 'braveWalletExploreDappsShowLess'
            : 'braveWalletExploreDappsShowMore'
        )}
      </ListToggleButton>
    </CategoryWrapper>
  )
}
