// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// selectors
import { useSafeUISelector } from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// styles
import { ContentWrapper } from './explore-dapps.styles'

export const ExploreDapps = () => {
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  return (
    <ContentWrapper
      fullWidth={true}
      fullHeight={isPanel}
      justifyContent='flex-start'
      isPanel={isPanel}
    ></ContentWrapper>
  )
}
