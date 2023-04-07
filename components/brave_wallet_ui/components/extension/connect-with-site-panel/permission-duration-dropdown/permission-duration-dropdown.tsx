// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  DAppPermissionDurationOption
} from '../../../../constants/types'

// Options
import { DAppPermissionDurationOptions } from '../../../../options/dapp-permission-duration-options'

// Utils
import { getLocale } from '../../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  DropDownButton,
  DropDownIcon,
  DropDown,
  PermissionButton
} from './permission-duration-dropdown.style'

interface Props {
  selectedDuration: BraveWallet.PermissionLifetimeOption
  setSelectedDuration: (duration: BraveWallet.PermissionLifetimeOption) => void
}

export const PermissionDurationDropdown = (props: Props) => {
  const { selectedDuration, setSelectedDuration } = props

  // State
  const [showDropdown, setShowDropdown] = React.useState<boolean>(false)

  // Methods
  const onSelectAndHide = React.useCallback(
    (duration: BraveWallet.PermissionLifetimeOption) => {
      setSelectedDuration(duration)
      setShowDropdown(false)
    },
    [setSelectedDuration]
  )

  // Memos
  const selectedDurationInfo = React.useMemo(() => {
    return (
      DAppPermissionDurationOptions.find(
        (item) => item.id === selectedDuration
      ) ?? DAppPermissionDurationOptions[0]
    )
  }, [selectedDuration])

  return (
    <StyledWrapper>
      <DropDownButton onClick={() => setShowDropdown((prev) => !prev)}>
        {getLocale(selectedDurationInfo.name)}
        <DropDownIcon name="carat-down" isOpen={showDropdown} />
      </DropDownButton>
      {showDropdown && (
        <DropDown>
          {DAppPermissionDurationOptions.map(
            (duration: DAppPermissionDurationOption) => (
              <PermissionButton
                key={duration.id}
                onClick={() => onSelectAndHide(duration.id)}
              >
                {getLocale(duration.name)}
              </PermissionButton>
            )
          )}
        </DropDown>
      )}
    </StyledWrapper>
  )
}

export default PermissionDurationDropdown
