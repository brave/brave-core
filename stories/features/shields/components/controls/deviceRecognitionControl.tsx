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
} from '../../../../../src/features/shields'

// Group Components
import StaticList from '../list/static'

// Fake data
import { getLocale } from '../../fakeLocale'
import data from '../../fakeData'

// Helpers
import { getTabIndexValueBasedOnProps } from '../../helpers'

interface Props {
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  fingerprintingBlocked: number
}

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

  get tabIndex () {
    const { isBlockedListOpen, fingerprintingBlocked } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, fingerprintingBlocked)
  }

  onOpenDeviceRecognitionOpen = (event: React.MouseEvent<HTMLDivElement>) => {
    if (event.currentTarget) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ deviceRecognitionOpen: !this.state.deviceRecognitionOpen })
  }

  onOpenDeviceRecognitionOpenViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event) {
      if (event.key === ' ') {
        event.currentTarget.blur()
        this.props.setBlockedListOpen()
        this.setState({ deviceRecognitionOpen: !this.state.deviceRecognitionOpen })
      }
    }
  }

  render () {
    const { favicon, hostname, isBlockedListOpen, fingerprintingBlocked } = this.props
    const { deviceRecognitionOpen } = this.state
    return (
      <>
        <BlockedInfoRowForSelect>
          <BlockedInfoRowDataForSelect
            disabled={fingerprintingBlocked === 0}
            tabIndex={this.tabIndex}
            onClick={this.onOpenDeviceRecognitionOpen}
            onKeyDown={this.onOpenDeviceRecognitionOpenViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats>{fingerprintingBlocked > 99 ? '99+' : fingerprintingBlocked}</BlockedInfoRowStats>
          </BlockedInfoRowDataForSelect>
          <SelectBox disabled={isBlockedListOpen}>
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
              list={data.blockedFakeResources}
              onClose={this.onOpenDeviceRecognitionOpen}
            />
        }
      </>
    )
  }
}
