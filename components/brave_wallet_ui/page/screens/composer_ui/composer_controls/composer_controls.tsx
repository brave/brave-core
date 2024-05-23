// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Styled Components
import {
  FlipButton,
  FlipIcon,
  SettingsButton,
  SettingsIcon
} from './composer_controls.style'
import { Row } from '../../../../components/shared/style'

interface Props {
  onFlipAssets?: () => void
  flipAssetsDisabled?: boolean
  onOpenSettings?: () => void
}

export const ComposerControls = (props: Props) => {
  const { onFlipAssets, onOpenSettings, flipAssetsDisabled } = props

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
      {onOpenSettings && (
        <SettingsButton onClick={onOpenSettings}>
          <SettingsIcon />
        </SettingsButton>
      )}
    </Row>
  )
}
