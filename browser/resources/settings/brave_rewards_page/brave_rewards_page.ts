/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

'use strict'

import {loadTimeData} from '../i18n_setup.js'

import '//resources/js/cr.js';
import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {BraveRewardsBrowserProxyImpl} from './brave_rewards_browser_proxy.js'
import {getTemplate} from './brave_rewards_page.html.js'

const SettingsBraveRewardsPageBase = I18nMixin(PrefsMixin(PolymerElement))

interface AutoContributeMonthlyLimit {
  name: string,
  value: number
}

interface AdsSubdivisionTargeting {
  name: string,
  value: string
}

/**
 * 'settings-brave-rewards-page' is the settings page containing settings for
 * Brave Rewards.
 */
class SettingsBraveRewardsPage extends SettingsBraveRewardsPageBase {
  static get is() {
    return 'settings-brave-rewards-page'
  }

  static get template () {
    return getTemplate()
  }

  static get properties () {
    return {
      countryCode_: {
        type: String,
        value: ''
      },
      subdivisions_: {
        type: Array,
        value: []
      },
      maxAdsToDisplay_: {
        readOnly: true,
        type: Array,
        value() {
          return [
            { name: loadTimeData.getString('braveRewardsDefaultItem'), value: -1 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour0'), value: 0 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour1'), value: 1 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour2'), value: 2 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour3'), value: 3 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour4'), value: 4 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour5'), value: 5 },
            { name: loadTimeData.getString('braveRewardsMaxAdsPerHour10'), value: 10 }
          ]
        }
      },
      autoContributeMinVisitTimeOptions_: {
        readOnly: true,
        type: Array,
        value() {
          return [
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisitTime5'), value: 5 },
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisitTime8'), value: 8 },
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisitTime60'), value: 60 }
          ]
        }
      },
      autoContributeMinVisitsOptions_: {
        readOnly: true,
        type: Array,
        value() {
          return [
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisits1'), value: 1 },
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisits5'), value: 5 },
            { name: loadTimeData.getString('braveRewardsAutoContributeMinVisits10'), value: 10 }
          ]
        }
      },
      autoContributeMonthlyLimit_: {
        type: Array,
        value: []
      },
      shouldAllowAdsSubdivisionTargeting_: {
        type: Boolean,
        value: false
      },
      shouldShowAutoContributeSettings_: {
        type: Boolean,
        value: false
      },
      isRewardsEnabled_: {
        type: Boolean,
        value: false
      },
    }
  }

  private countryCode_: string
  private subdivisions_: AdsSubdivisionTargeting[]
  private isRewardsEnabled_: boolean
  private autoContributeMonthlyLimit_: AutoContributeMonthlyLimit[]
  private shouldAllowAdsSubdivisionTargeting_: boolean
  private shouldShowAutoContributeSettings_: boolean
  private browserProxy_ = BraveRewardsBrowserProxyImpl.getInstance()

  override ready () {
    super.ready()
    this.browserProxy_.getRewardsEnabled().then((enabled) => {
      if (enabled) {
        this.onRewardsEnabled_()
      }
    })
    chrome.braveRewards.onRewardsWalletUpdated.addListener(() => {
      // If Rewards hasn't been enabled before now, we know it is now so trigger
      // its handler
      if (!this.isRewardsEnabled_) {
        this.onRewardsEnabled_()
      }
    })
    chrome.braveRewards.onExternalWalletConnected.addListener(() => {
      this.maybeShowAutoContributeSettings_()
    })
    chrome.settingsPrivate.onPrefsChanged.addListener((prefs) => {
      prefs.forEach((pref) => {
        if (pref.key === 'brave.brave_ads.should_allow_ads_subdivision_targeting') {
          this.getAdsDataForSubdivisionTargeting_()
        }
      }, this)
    })
  }

  private openRewardsPanel_ () {
    chrome.braveRewards.openRewardsPanel()
    this.maybeShowAutoContributeSettings_()
  }

  private async maybeShowAutoContributeSettings_ () {
    // Show auto-contribute settings if applicable and user is connected
    try {
      const supportsAutoContribute =
        await this.browserProxy_.isAutoContributeSupported()
      const userType = await this.browserProxy_.getUserType()
      this.shouldShowAutoContributeSettings_ =
        supportsAutoContribute && userType !== 'unconnected'
    } catch (e) {
      console.error(e)
    }
  }

  private onRewardsEnabled_ () {
    this.isRewardsEnabled_ = true
    this.maybeShowAutoContributeSettings_()
    this.populateAutoContributeAmountDropdown_()
    this.getAdsDataForSubdivisionTargeting_()
  }

  private getAdsDataForSubdivisionTargeting_ () {
    this.browserProxy_.getAdsData().then((adsData: any) => {
      this.shouldAllowAdsSubdivisionTargeting_ = adsData.shouldAllowAdsSubdivisionTargeting
      this.countryCode_ = adsData.countryCode
      this.subdivisions_ = adsData.subdivisions
    })
  }

  private populateAutoContributeAmountDropdown_ () {
    this.browserProxy_.getRewardsParameters().then((params: any) => {
      let autoContributeChoices = [
        { name: loadTimeData.getString('braveRewardsDefaultItem'), value: 0 }
      ]
      params.autoContributeChoices.forEach((element: number) => {
        autoContributeChoices.push({
          name: `${loadTimeData.getString('braveRewardsContributionUpTo')} ${element.toFixed(3)} BAT`,
          value: element
        })
      })
      this.autoContributeMonthlyLimit_ = autoContributeChoices
    })
  }

  private shouldShowRestartButtonForTipButtons_(
    buttonsEnabled: boolean,
    redditEnabled: boolean,
    twitterEnabled: boolean,
    githubEnabled: boolean) {
    return (
      buttonsEnabled !== this.browserProxy_.wasInlineTipButtonsEnabledAtStartup() ||
      redditEnabled !== this.browserProxy_.wasInlineTipRedditEnabledAtStartup() ||
      twitterEnabled !== this.browserProxy_.wasInlineTipTwitterEnabledAtStartup() ||
      githubEnabled !== this.browserProxy_.wasInlineTipGithubEnabledAtStartup())
  }

  private restartBrowser_ (e: Event) {
    e.stopPropagation()
    window.open('chrome://restart', '_self')
  }

  private adsSubdivisionTargetingCodes_ () {
    if (!this.subdivisions_ || !this.subdivisions_.length) {
      return []
    }

    let subdivisions = this.subdivisions_.map(val => ({ ...val }))
    subdivisions.unshift({ name: loadTimeData.getString('braveRewardsDisabledItem'), value: 'DISABLED' })
    subdivisions.unshift({ name: loadTimeData.getString('braveRewardsAutoDetectedItem'), value: 'AUTO' })
    return subdivisions
  }
}

customElements.define(
  SettingsBraveRewardsPage.is, SettingsBraveRewardsPage)
