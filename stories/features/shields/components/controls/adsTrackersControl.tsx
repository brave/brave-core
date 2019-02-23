/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRow,
  BlockedInfoRowData,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedInfoRowText,
  Toggle
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
  adsTrackersBlocked: number
}

interface State {
  trackersBlockedOpen: boolean
  trackersBlockedEnabled: boolean
}

export default class AdsTrackersControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      trackersBlockedEnabled: true,
      trackersBlockedOpen: false
    }
  }

  get tabIndex () {
    const { isBlockedListOpen, adsTrackersBlocked } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, adsTrackersBlocked)
  }

  onOpen3rdPartyTrackersBlocked = (event: React.MouseEvent<HTMLDivElement>) => {
    if (event.currentTarget) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ trackersBlockedOpen: !this.state.trackersBlockedOpen })
  }

  onOpen3rdPartyTrackersBlockedViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event) {
      if (event.key === ' ') {
        event.currentTarget.blur()
        this.props.setBlockedListOpen()
        this.setState({ trackersBlockedOpen: !this.state.trackersBlockedOpen })
      }
    }
  }

  onChange3rdPartyTrackersBlockedEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ trackersBlockedEnabled: event.target.checked })
  }

  render () {
    const { favicon, hostname, adsTrackersBlocked, isBlockedListOpen } = this.props
    const { trackersBlockedOpen, trackersBlockedEnabled } = this.state
    return (
      <>
        <BlockedInfoRow>
          <BlockedInfoRowData
            disabled={adsTrackersBlocked === 0}
            tabIndex={this.tabIndex}
            onClick={this.onOpen3rdPartyTrackersBlocked}
            onKeyDown={this.onOpen3rdPartyTrackersBlockedViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats>{adsTrackersBlocked > 99 ? '99+' : adsTrackersBlocked}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('thirdPartyTrackersBlocked')}</BlockedInfoRowText>
          </BlockedInfoRowData>
          <Toggle
            size='small'
            disabled={isBlockedListOpen}
            checked={trackersBlockedEnabled}
            onChange={this.onChange3rdPartyTrackersBlockedEnabled}
          />
        </BlockedInfoRow>
        {
          trackersBlockedOpen &&
            <StaticList
              favicon={favicon}
              hostname={hostname}
              stats={adsTrackersBlocked}
              name={getLocale('thirdPartyTrackersBlocked')}
              list={data.blockedFakeResources}
              onClose={this.onOpen3rdPartyTrackersBlocked}
            />
        }
      </>
    )
  }
}
