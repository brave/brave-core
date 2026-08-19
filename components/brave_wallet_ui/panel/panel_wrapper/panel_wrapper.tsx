// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useSafeUISelector } from '../../common/hooks/use-safe-selector'
import { UISelectors } from '../../common/selectors'

// Styled Components
import { Wrapper, SidePanelWrapper } from './panel_wrapper.style'

interface Props {
  children: React.ReactNode
}

export const PanelWrapper = (props: Props) => {
  const { children } = props

  // UI Selectors
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)

  // Wrapper for side panel
  if (isSidePanel) {
    return <SidePanelWrapper>{children}</SidePanelWrapper>
  }

  // Wrapper for main panel
  return <Wrapper>{children}</Wrapper>
}
