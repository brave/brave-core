/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  ShieldsHeader,
  MainToggle,
  TotalBlockedStats,
  SiteOverview,
  SiteInfo,
  MainToggleHeading,
  MainToggleText,
  Link,
  ToggleStateText,
  Favicon,
  SiteInfoText,
  TotalBlockedStatsNumber,
  TotalBlockedStatsText,
  DisabledContentView,
  ShieldIcon,
  DisabledContentText,
  Toggle
 } from 'brave-ui/features/shields'

// Helpers
import { getLocale } from '../../background/api/localeAPI'
import {
  blockedResourcesSize,
  getToggleStateViaEventTarget
} from '../../helpers/shieldsUtils'

// Types
import { BlockOptions } from '../../types/other/blockTypes'
import { ShieldsToggled } from '../../types/actions/shieldsPanelActions'

interface CommonProps {
  enabled: boolean
  favicon: string
  hostname: string
  shieldsToggled: ShieldsToggled
  toggleReadOnlyView: () => void
}

interface BlockedItemsProps {
  adsBlocked: number
  trackersBlocked: number
  httpsUpgrades: number
  scriptsBlocked: number
  fingerprintingBlocked: number
}

export type Props = CommonProps & BlockedItemsProps

export default class Header extends React.PureComponent<Props, {}> {
  get blockedItemsSize (): number {
    const { adsBlocked, trackersBlocked, scriptsBlocked, fingerprintingBlocked } = this.props
    return adsBlocked + trackersBlocked + scriptsBlocked + fingerprintingBlocked
  }

  get totalBlockedSize (): string {
    const { httpsUpgrades } = this.props
    const total = this.blockedItemsSize + httpsUpgrades
    return blockedResourcesSize(total)
  }

  onToggleShields = (event: React.ChangeEvent<HTMLInputElement>) => {
    const shieldsOption: BlockOptions = getToggleStateViaEventTarget(event)
    this.props.shieldsToggled(shieldsOption)
  }

  render () {
    const { enabled, favicon, hostname, toggleReadOnlyView } = this.props
    return (
      <ShieldsHeader status={enabled ? 'enabled' : 'disabled'}>
        <MainToggle status={enabled ? 'enabled' : 'disabled'}>
          <div>
            <MainToggleHeading>
              {getLocale('shields')}
              <ToggleStateText status={enabled ? 'enabled' : 'disabled'}>
                {enabled ? ` ${getLocale('up')} ` : ` ${getLocale('down')} `}
              </ToggleStateText>
              {getLocale('forThisSite')}
            </MainToggleHeading>
            {enabled ? <MainToggleText>{getLocale('enabledMessage')}</MainToggleText> : null}
          </div>
          <Toggle id='mainToggle' size='large' checked={enabled} onChange={this.onToggleShields} />
        </MainToggle>
        <SiteOverview status={enabled ? 'enabled' : 'disabled'}>
          <SiteInfo>
            <Favicon src={favicon} />
            <SiteInfoText>{hostname}</SiteInfoText>
          </SiteInfo>
          {
            enabled
            ? (
              <TotalBlockedStats size='large'>
                <TotalBlockedStatsNumber>{this.totalBlockedSize}</TotalBlockedStatsNumber>
                <TotalBlockedStatsText>
                  {`${getLocale('blockedResoucesExplanation')} `}
                  <Link onClick={toggleReadOnlyView}>{getLocale('learnMore')}</Link>
                </TotalBlockedStatsText>
              </TotalBlockedStats>
            )
            : (
              <DisabledContentView>
                <div><ShieldIcon /></div>
                <DisabledContentText>{getLocale('disabledMessage')}</DisabledContentText>
              </DisabledContentView>
            )
          }
        </SiteOverview>
      </ShieldsHeader>
    )
  }
}
