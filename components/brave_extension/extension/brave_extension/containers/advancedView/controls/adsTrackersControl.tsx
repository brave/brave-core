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
  sumAdsAndTrackers,
  mergeAdsAndTrackersResources,
  maybeBlockResource,
  getTabIndexValueBasedOnProps,
  blockedResourcesSize,
  shouldDisableResourcesRow
} from '../../../helpers/shieldsUtils'

// Types
import { BlockFPOptions, BlockOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface AdsTrackersProps {
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: string[]
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: string[]
  firstPartyCosmeticFiltering: boolean
  blockAdsTrackers: (event: string) => void
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

  get totalAdsTrackersBlockedList (): string[] {
    const { adsBlockedResources, trackersBlockedResources } = this.props
    return mergeAdsAndTrackersResources(adsBlockedResources, trackersBlockedResources)
  }

  get maybeBlock3rdPartyTrackersBlocked (): BlockFPOptions {
    const { ads, trackers, firstPartyCosmeticFiltering } = this.props
    if (!(maybeBlockResource(ads) && maybeBlockResource(trackers))) {
      return 'allow'
    }
    return firstPartyCosmeticFiltering ? 'block' : 'block_third_party'
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

  onChange3rdPartyTrackersBlockedEnabled = (event: React.ChangeEvent<HTMLSelectElement>) => {
    this.props.blockAdsTrackers(event.target.value)
  }

  render () {
    const { favicon, hostname, isBlockedListOpen } = this.props
    const { trackersBlockedOpen } = this.state
    return (
      <>
        <BlockedInfoRowForSelect id='adsTrackersControl'>
          <BlockedInfoRowDataForSelect
            disabled={this.shouldDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpen3rdPartyTrackersBlocked}
            onKeyDown={this.onOpen3rdPartyTrackersBlockedViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='blockAdsStat'>{this.totalAdsTrackersBlockedDisplay}</BlockedInfoRowStats>
          </BlockedInfoRowDataForSelect>
          <SelectBox
            id='blockAds'
            disabled={isBlockedListOpen}
            value={this.maybeBlock3rdPartyTrackersBlocked}
            onChange={this.onChange3rdPartyTrackersBlockedEnabled}
          >
            <option value='block'>{getLocale('aggressiveAdsTrackersBlocking')}</option>
            <option value='block_third_party'>{getLocale('standardAdsTrackersBlocking')}</option>
            <option value='allow'>{getLocale('allowAdsTrackers')}</option>
          </SelectBox>
        </BlockedInfoRowForSelect>
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
