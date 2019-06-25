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
 } from '../../../../../src/features/shields'

// Fake data
import { getLocale } from '../../fakeLocale'

interface Props {
  enabled: boolean
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  adsTrackersBlocked: number
  scriptsBlocked: number
  httpsUpgrades: number
  fingerprintingBlocked: number
  fakeOnChangeShieldsEnabled: () => void
  fakeOnChangeReadOnlyView: () => void
}

export default class Header extends React.PureComponent<Props, {}> {
  get totalBlocked () {
    const { adsTrackersBlocked, httpsUpgrades, scriptsBlocked, fingerprintingBlocked } = this.props
    const total = adsTrackersBlocked + httpsUpgrades + scriptsBlocked + fingerprintingBlocked
    if (!total) {
      return 0
    }
    return total > 99 ? '99+' : total
  }

  render () {
    const {
      enabled,
      favicon,
      hostname,
      isBlockedListOpen,
      fakeOnChangeReadOnlyView,
      fakeOnChangeShieldsEnabled
    } = this.props
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
          <Toggle size='large' checked={enabled} onChange={fakeOnChangeShieldsEnabled} disabled={isBlockedListOpen} />
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
                <TotalBlockedStatsNumber>{this.totalBlocked}</TotalBlockedStatsNumber>
                <TotalBlockedStatsText>
                  {`${getLocale('blockedResoucesExplanation')} `}
                  <Link onClick={fakeOnChangeReadOnlyView}>{getLocale('learnMore')}</Link>
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
