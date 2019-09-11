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
} from '../../../components'

// Group Components
import HTTPSUpgrades from '../overlays/httpsUpgradesOverlay'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  maybeBlockResource,
  shouldDisableResourcesRow,
  getTabIndexValueBasedOnProps,
  blockedResourcesSize,
  getToggleStateViaEventTarget
} from '../../../helpers/shieldsUtils'

// Types
import { HttpsEverywhereToggled } from '../../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface HTTPSUpgradesProps {
  httpsRedirected: number
  httpUpgradableResources: BlockOptions
  httpsRedirectedResources: Array<string>
  httpsEverywhereToggled: HttpsEverywhereToggled
}

export type Props = CommonProps & HTTPSUpgradesProps

interface State {
  connectionsUpgradedOpen: boolean
}

export default class HTTPSUpgradesControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { connectionsUpgradedOpen: false }
  }

  get shouldDisableResourcesRow (): boolean {
    const { httpsRedirected } = this.props
    return shouldDisableResourcesRow(httpsRedirected)
  }

  get httpsRedirectedDisplay (): string {
    const { httpsRedirected } = this.props
    return blockedResourcesSize(httpsRedirected)
  }

  get tabIndex (): number {
    const { isBlockedListOpen, httpsRedirected } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, httpsRedirected)
  }

  get shouldBlockConnectionsUpgradedToHTTPS (): boolean {
    const { httpUpgradableResources } = this.props
    return maybeBlockResource(httpUpgradableResources)
  }

  triggerConnectionsUpgradedToHTTPS = (
    event: React.MouseEvent<HTMLDivElement> | React.KeyboardEvent<HTMLDivElement>
  ) => {
    if (event) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ connectionsUpgradedOpen: !this.state.connectionsUpgradedOpen })
  }

  onOpenConnectionsUpgradedToHTTPS = (event: React.MouseEvent<HTMLDivElement>) => {
    this.triggerConnectionsUpgradedToHTTPS(event)
  }

  onOpenConnectionsUpgradedToHTTPSViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event.key === ' ') {
      this.triggerConnectionsUpgradedToHTTPS(event)
    }
  }

  onChangeConnectionsUpgradedToHTTPSEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    const shouldEnableHttpsEverywhere = getToggleStateViaEventTarget(event)
    this.props.httpsEverywhereToggled(shouldEnableHttpsEverywhere)
  }

  render () {
    const { isBlockedListOpen, favicon, hostname, httpsRedirected, httpsRedirectedResources } = this.props
    const { connectionsUpgradedOpen } = this.state
    return (
      <>
        <BlockedInfoRow id='httpsUpgradesControl'>
          <BlockedInfoRowData
            disabled={this.shouldDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpenConnectionsUpgradedToHTTPS}
            onKeyDown={this.onOpenConnectionsUpgradedToHTTPSViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='connectionsEncryptedStat'>{this.httpsRedirectedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('connectionsUpgradedHTTPS')}</BlockedInfoRowText>
          </BlockedInfoRowData>
          <Toggle
            id='connectionsEncrypted'
            size='small'
            disabled={isBlockedListOpen}
            checked={this.shouldBlockConnectionsUpgradedToHTTPS}
            onChange={this.onChangeConnectionsUpgradedToHTTPSEnabled}
          />
        </BlockedInfoRow>
        {
          connectionsUpgradedOpen &&
            <HTTPSUpgrades
              favicon={favicon}
              hostname={hostname}
              stats={httpsRedirected}
              list={httpsRedirectedResources}
              onClose={this.onOpenConnectionsUpgradedToHTTPS}
            />
        }
      </>
    )
  }
}
