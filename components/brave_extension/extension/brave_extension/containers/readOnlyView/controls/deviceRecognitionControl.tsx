/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowDetails,
  ArrowUpIcon,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedInfoRowSummary,
  BlockedInfoRowData,
  BlockedListStatic,
  BlockedInfoRowText
} from '../../../components'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  blockedResourcesSize,
  shouldDisableResourcesRow
} from '../../../helpers/shieldsUtils'

// Types
import { BlockFPOptions } from '../../../types/other/blockTypes'

interface Props {
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  fingerprintingBlockedResources: Array<string>
}

interface State {
  deviceRecognitionOpen: boolean
}

export default class DeviceRecognitionControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { deviceRecognitionOpen: false }
  }

  get totalDeviceRecognitonAttemptsDisplay (): string {
    const { fingerprintingBlocked } = this.props
    return blockedResourcesSize(fingerprintingBlocked)
  }

  get shouldDisableResourcesRow (): boolean {
    const { fingerprintingBlocked } = this.props
    return shouldDisableResourcesRow(fingerprintingBlocked)
  }

  triggerOpenDeviceRecognition = () => {
    if (!this.shouldDisableResourcesRow) {
      this.setState({ deviceRecognitionOpen: !this.state.deviceRecognitionOpen })
    }
  }

  render () {
    const { deviceRecognitionOpen } = this.state
    const { fingerprintingBlockedResources } = this.props
    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.triggerOpenDeviceRecognition}>
          <BlockedInfoRowData disabled={this.shouldDisableResourcesRow}>
            {
              deviceRecognitionOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{this.totalDeviceRecognitonAttemptsDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('readOnlyFingerprinting')}</BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={fingerprintingBlockedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
