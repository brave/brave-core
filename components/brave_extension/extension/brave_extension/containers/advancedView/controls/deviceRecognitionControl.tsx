/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowForSelect,
  BlockedInfoRowDataForSelect,
  ArrowDownIcon,
  BlockedInfoRowStats,
  SelectBox
} from '../../../components'

// Group Components
import StaticList from '../overlays/staticOverlay'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  shouldDisableResourcesRow,
  getTabIndexValueBasedOnProps,
  blockedResourcesSize
} from '../../../helpers/shieldsUtils'

// Types
import { BlockFPOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
}

interface DeviceRecognitionProps {
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  fingerprintingBlockedResources: Array<string>
  blockFingerprinting: (event: string) => void
}

export type Props = CommonProps & DeviceRecognitionProps

interface State {
  deviceRecognitionOpen: boolean
}

export default class DeviceRecognitionControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      deviceRecognitionOpen: false
    }
  }

  get totalDeviceRecognitonAttemptsDisplay (): string {
    const { fingerprintingBlocked } = this.props
    return blockedResourcesSize(fingerprintingBlocked)
  }

  get shouldDisableResourcesRow (): boolean {
    const { fingerprintingBlocked } = this.props
    return shouldDisableResourcesRow(fingerprintingBlocked)
  }

  get tabIndex (): number {
    const { isBlockedListOpen, fingerprintingBlocked } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, fingerprintingBlocked)
  }

  triggerOpenDeviceRecognition = (
    event: React.MouseEvent<HTMLDivElement> | React.KeyboardEvent<HTMLDivElement>
  ) => {
    if (event) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ deviceRecognitionOpen: !this.state.deviceRecognitionOpen })
  }

  onOpenDeviceRecognition = (event: React.MouseEvent<HTMLDivElement>) => {
    this.triggerOpenDeviceRecognition(event)
  }

  onOpenDeviceRecognitionViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event.key === ' ') {
      this.triggerOpenDeviceRecognition(event)
    }
  }

  onChangeBlockDeviceRecognition = (event: React.ChangeEvent<HTMLSelectElement>) => {
    this.props.blockFingerprinting(event.target.value)
  }

  render () {
    const {
      favicon,
      hostname,
      isBlockedListOpen,
      fingerprintingBlocked,
      fingerprintingBlockedResources,
      fingerprinting
    } = this.props
    const { deviceRecognitionOpen } = this.state
    return (
      <>
        <BlockedInfoRowForSelect id='deviceRecognitionControl'>
          <BlockedInfoRowDataForSelect
            disabled={this.shouldDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpenDeviceRecognition}
            onKeyDown={this.onOpenDeviceRecognitionViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='blockFingerprintingStat'>{this.totalDeviceRecognitonAttemptsDisplay}</BlockedInfoRowStats>
          </BlockedInfoRowDataForSelect>
          <SelectBox
            id='blockFingerprinting'
            disabled={isBlockedListOpen}
            value={fingerprinting}
            onChange={this.onChangeBlockDeviceRecognition}
          >
            <option value='block_third_party'>{getLocale('thirdPartyFingerprintingBlocked')}</option>
            <option value='block'>{getLocale('allFingerprintingBlocked')}</option>
            <option value='allow'>{getLocale('allFingerprintingAllowed')}</option>
          </SelectBox>
        </BlockedInfoRowForSelect>
        {
          deviceRecognitionOpen &&
            <StaticList
              favicon={favicon}
              hostname={hostname}
              stats={fingerprintingBlocked}
              name={getLocale('deviceRecognitionAttempts')}
              list={fingerprintingBlockedResources}
              onClose={this.onOpenDeviceRecognition}
            />
        }
      </>
    )
  }
}
