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

interface Props {
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  httpsUpgrades: number
}

interface State {
  connectionsUpgradedEnabled: boolean
  connectionsUpgradedOpen: boolean
}

export default class HTTPSUpgradesControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      connectionsUpgradedEnabled: true,
      connectionsUpgradedOpen: false
    }
  }

  get tabIndex () {
    const { isBlockedListOpen } = this.props
    return isBlockedListOpen ? -1 : 0
  }

  // HTTPS
  onOpenConnectionsUpgradedToHTTPS = (event: React.MouseEvent<HTMLDivElement>) => {
    if (event.currentTarget) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ connectionsUpgradedOpen: !this.state.connectionsUpgradedOpen })
    // setNativeProps
  }

  onOpenConnectionsUpgradedToHTTPSViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event) {
      if (event.key === ' ') {
        event.currentTarget.blur()
        this.props.setBlockedListOpen()
        this.setState({ connectionsUpgradedOpen: !this.state.connectionsUpgradedOpen })
      }
    }
  }

  onChangeConnectionsUpgradedToHTTPSEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ connectionsUpgradedEnabled: event.target.checked })
  }

  render () {
    const { isBlockedListOpen, favicon, hostname, httpsUpgrades } = this.props
    const { connectionsUpgradedOpen, connectionsUpgradedEnabled } = this.state
    return (
      <>
        <BlockedInfoRow>
          <BlockedInfoRowData
            tabIndex={this.tabIndex}
            onClick={this.onOpenConnectionsUpgradedToHTTPS}
            onKeyDown={this.onOpenConnectionsUpgradedToHTTPSViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats>{httpsUpgrades > 99 ? '99+' : httpsUpgrades}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('connectionsUpgradedHTTPSCapital')}</BlockedInfoRowText>
          </BlockedInfoRowData>
          <Toggle
            size='small'
            disabled={isBlockedListOpen}
            checked={connectionsUpgradedEnabled}
            onChange={this.onChangeConnectionsUpgradedToHTTPSEnabled}
          />
        </BlockedInfoRow>
        {
          connectionsUpgradedOpen &&
            <StaticList
              favicon={favicon}
              hostname={hostname}
              stats={httpsUpgrades}
              name={getLocale('connectionsUpgradedHTTPSCapital')}
              list={data.blockedFakeResources}
              onClose={this.onOpenConnectionsUpgradedToHTTPS}
            />
        }
      </>
    )
  }
}
