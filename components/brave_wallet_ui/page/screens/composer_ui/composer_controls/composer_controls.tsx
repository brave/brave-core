// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Utils
import { getLocale } from '../../../../../common/locale'

// Types
import { NavOption } from '../../../../constants/types'

// Options
import { SendSwapBridgeOptions } from '../../../../options/nav-options'

// Styled Components
import {
  ComposerButton,
  ComposerButtonMenu,
  FlipButton,
  FlipIcon,
  SettingsButton,
  SettingsIcon,
  CaratIcon
} from './composer_controls.style'
import { Row } from '../../../../components/shared/style'

interface Props {
  onFlipAssets?: () => void
  onOpenSettings?: () => void
}

export const ComposerControls = (props: Props) => {
  const { onFlipAssets, onOpenSettings } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

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

  // Methods
  const onChange = (option?: NavOption) => {
    if (showComposerMenu && option) {
      history.push(option.route)
    }
    setShowComposerMenu((prev) => !prev)
  }

  // Computed
  const selectedOption = SendSwapBridgeOptions.find((option) =>
    walletLocation.includes(option.route)
  )

  // Moves the selectedOption to the front of the list.
  const buttonOptions = showComposerMenu
    ? [
        SendSwapBridgeOptions.find(
          (option) => option.id === selectedOption?.id
        ),
        ...SendSwapBridgeOptions.filter(
          (option) => option.id !== selectedOption?.id
        )
      ]
    : [selectedOption]

  return (
    <Row>
      {onFlipAssets && (
        <FlipButton onClick={onFlipAssets}>
          <FlipIcon />
        </FlipButton>
      )}
      {buttonOptions ? (
        <ComposerButtonMenu ref={buttonMenuRef}>
          {buttonOptions.map((option, i) => (
            <ComposerButton
              key={option?.id}
              onClick={() => onChange(option)}
            >
              {getLocale(option?.name ?? '')}
              {i === 0 && <CaratIcon isOpen={showComposerMenu} />}
            </ComposerButton>
          ))}
        </ComposerButtonMenu>
      ) : null}
      {onOpenSettings && (
        <SettingsButton onClick={onOpenSettings}>
          <SettingsIcon />
        </SettingsButton>
      )}
    </Row>
  )
}
