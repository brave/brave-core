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
import StaticList from '../overlays/staticOverlay'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  sumAdsAndTrackers,
  mergeAdsAndTrackersResources,
  maybeBlockResource,
  getToggleStateViaEventTarget,
  getTabIndexValueBasedOnProps,
  blockedResourcesSize,
  shouldDisableResourcesRow
} from '../../../helpers/shieldsUtils'

// Types
import { BlockAdsTrackers } from '../../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface AdsTrackersProps {
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: Array<string>
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: Array<string>
  blockAdsTrackers: BlockAdsTrackers
}

export type Props = CommonProps & AdsTrackersProps

interface State {
  trackersBlockedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { trackersBlockedOpen: false }
  }

  get totalAdsTrackersBlocked (): number {
    const { adsBlocked, trackersBlocked } = this.props
    return sumAdsAndTrackers(adsBlocked, trackersBlocked)
  }

  get totalAdsTrackersBlockedDisplay (): string {
    return blockedResourcesSize(this.totalAdsTrackersBlocked)
  }

  get totalAdsTrackersBlockedList (): Array<string> {
    const { adsBlockedResources, trackersBlockedResources } = this.props
    return mergeAdsAndTrackersResources(adsBlockedResources, trackersBlockedResources)
  }

  get maybeBlock3rdPartyTrackersBlocked (): boolean {
    const { ads, trackers } = this.props
    return maybeBlockResource(ads) && maybeBlockResource(trackers)
  }

  get shouldDisableResourcesRow (): boolean {
    return shouldDisableResourcesRow(this.totalAdsTrackersBlocked)
  }

  get tabIndex (): number {
    const { isBlockedListOpen } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, this.totalAdsTrackersBlocked)
  }

  triggerOpen3rdPartyTrackersBlocked = (
    event: React.MouseEvent<HTMLDivElement> | React.KeyboardEvent<HTMLDivElement>
  ) => {
    if (event) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ trackersBlockedOpen: !this.state.trackersBlockedOpen })
  }

  onOpen3rdPartyTrackersBlocked = (event: React.MouseEvent<HTMLDivElement>) => {
    this.triggerOpen3rdPartyTrackersBlocked(event)
  }

  onOpen3rdPartyTrackersBlockedViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event.key === ' ') {
      this.triggerOpen3rdPartyTrackersBlocked(event)
    }
  }

  onChange3rdPartyTrackersBlockedEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    const shoudEnableAdsTracks = getToggleStateViaEventTarget(event)
    this.props.blockAdsTrackers(shoudEnableAdsTracks)
  }

  render () {
    const { favicon, hostname, isBlockedListOpen } = this.props
    const { trackersBlockedOpen } = this.state
    return (
      <>
        <BlockedInfoRow id='adsTrackersControl'>
          <BlockedInfoRowData
            disabled={this.shouldDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpen3rdPartyTrackersBlocked}
            onKeyDown={this.onOpen3rdPartyTrackersBlockedViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='blockAdsStat'>{this.totalAdsTrackersBlockedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('thirdPartyTrackersBlocked')}</BlockedInfoRowText>
          </BlockedInfoRowData>
          <Toggle
            id='blockAds'
            size='small'
            disabled={isBlockedListOpen}
            checked={this.maybeBlock3rdPartyTrackersBlocked}
            onChange={this.onChange3rdPartyTrackersBlockedEnabled}
          />
        </BlockedInfoRow>
        {
          trackersBlockedOpen &&
            <StaticList
              favicon={favicon}
              hostname={hostname}
              stats={this.totalAdsTrackersBlocked}
              name={getLocale('thirdPartyTrackersBlocked')}
              list={this.totalAdsTrackersBlockedList}
              onClose={this.onOpen3rdPartyTrackersBlocked}
            />
        }
      </>
    )
  }
}
