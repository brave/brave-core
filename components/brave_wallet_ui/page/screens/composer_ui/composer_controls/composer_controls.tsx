// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import { SwapProviderMetadata } from '../../swap/constants/metadata'

// Types
import { BraveWallet } from '../../../../constants/types'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Styled Components
import {
  FlipButton,
  FlipIcon,
  ProvidersButton,
  ProviderIcon
} from './composer_controls.style'
import { Row } from '../../../../components/shared/style'

interface Props {
  onFlipAssets: () => void
  onOpenProviders: () => void
  selectedProvider: BraveWallet.SwapProvider
  flipAssetsDisabled?: boolean
}

export const ComposerControls = (props: Props) => {
  const {
    onFlipAssets,
    onOpenProviders,
    flipAssetsDisabled,
    selectedProvider
  } = props

  // State
  const [showComposerMenu, setShowComposerMenu] = React.useState<boolean>(false)

  // Refs
  const buttonMenuRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    buttonMenuRef,
    () => setShowComposerMenu(false),
    showComposerMenu
  )

  return (
    <Row>
      {onFlipAssets && (
        <FlipButton
          onClick={onFlipAssets}
          disabled={flipAssetsDisabled}
        >
          <FlipIcon />
        </FlipButton>
      )}
      {onOpenProviders && (
        <ProvidersButton onClick={onOpenProviders}>
          <ProviderIcon src={SwapProviderMetadata[selectedProvider]} />
        </ProvidersButton>
      )}
    </Row>
  )
}
