/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

'use strict';

import {loadTimeData} from '../i18n_setup.js';

import '//resources/js/cr.m.js';
import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import {BraveRewardsBrowserProxyImpl} from './brave_rewards_browser_proxy.js';
import {getTemplate} from './brave_rewards_page.html.js'

const SettingsBraveRewardsPageBase = I18nMixin(PrefsMixin(PolymerElement))

/**
 * 'settings-brave-rewards-page' is the settings page containing settings for
 * Brave Rewards.
 */
class SettingsBraveRewardsPage extends SettingsBraveRewardsPageBase {
  static get is() {
    return 'settings-brave-rewards-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
        value: true
      },
      isRewardsEnabled_: {
        type: Boolean,
        value: false
      },
      wasInlineTippingForRedditEnabledOnStartup_: {
        type: Boolean,
        value: false,
      },
      wasInlineTippingForTwitterEnabledOnStartup_: {
        type: Boolean,
        value: false,
      },
      wasInlineTippingForGithubEnabledOnStartup_: {
        type: Boolean,
        value: false,
      }
    }
  }

  browserProxy_ = BraveRewardsBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.openRewardsPanel_ = () => {
      chrome.braveRewards.openRewardsPanel()
      this.isAutoContributeSupported_()
    }
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
    chrome.settingsPrivate.onPrefsChanged.addListener((prefs) => {
      prefs.forEach((pref) => {
        if (pref.key === 'brave.brave_ads.should_allow_ads_subdivision_targeting') {
          this.getAdsDataForSubdivisionTargeting_()
        }
      }, this)
    })
  }

  isAutoContributeSupported_() {
    this.browserProxy_.isAutoContributeSupported().then((supported) => {
      // Show auto-contribute settings if this profile supports it
      this.shouldShowAutoContributeSettings_ = supported
    })
  }

  onRewardsEnabled_() {
    this.isRewardsEnabled_ = true
    this.wasInlineTippingForRedditEnabledOnStartup_ = this.getPref('brave.rewards.inline_tip.reddit').value
    this.wasInlineTippingForTwitterEnabledOnStartup_ = this.getPref('brave.rewards.inline_tip.twitter').value
    this.wasInlineTippingForGithubEnabledOnStartup_ = this.getPref('brave.rewards.inline_tip.github').value
    this.isAutoContributeSupported_()
    this.populateAutoContributeAmountDropdown_()
    this.getAdsDataForSubdivisionTargeting_()
  }

  getAdsDataForSubdivisionTargeting_() {
    this.browserProxy_.getAdsData().then((adsData) => {
      this.shouldAllowAdsSubdivisionTargeting_ = adsData.shouldAllowAdsSubdivisionTargeting
      this.countryCode_ = adsData.countryCode
    })
  }

  populateAutoContributeAmountDropdown_() {
    this.browserProxy_.getRewardsParameters().then((params) => {
      let autoContributeChoices = [
        { name: loadTimeData.getString('braveRewardsDefaultItem'), value: 0 }
      ]
      params.autoContributeChoices.forEach((element) => {
        autoContributeChoices.push({
          name: `${loadTimeData.getString('braveRewardsContributionUpTo')} ${element.toFixed(3)} BAT`,
          value: element
        })
      })
      this.autoContributeMonthlyLimit_ = autoContributeChoices
    })
  }

  shouldShowRestartButtonForReddit_(enabled) {
    if (!this.isRewardsEnabled_) {
      return false
    }

    return enabled !== this.wasInlineTippingForRedditEnabledOnStartup_
  }

  shouldShowRestartButtonForTwitter_(enabled) {
    if (!this.isRewardsEnabled_) {
      return false
    }

    return enabled !== this.wasInlineTippingForTwitterEnabledOnStartup_
  }

  shouldShowRestartButtonForGithub_(enabled) {
    if (!this.isRewardsEnabled_) {
      return false
    }

    return enabled !== this.wasInlineTippingForGithubEnabledOnStartup_
  }

  restartBrowser_(e) {
    e.stopPropagation();
    window.open('chrome://restart', '_self');
  }

  adsSubdivisionTargetingCodes_() {
    if (this.countryCode_ == 'US') {
      return this.adsUSASubdivisionTargetingCodes_()
    }

    if (this.countryCode_ == 'CA') {
      return this.adsCanadaSubdivisionTargetingCodes_()
    }

    return []
  }

  adsUSASubdivisionTargetingCodes_() {
    return [
      { name: loadTimeData.getString('braveRewardsAutoDetectedItem'), value: 'AUTO' },
      { name: loadTimeData.getString('braveRewardsDisabledItem'), value: 'DISABLED' },
      { name: 'Alabama', value: 'US-AL' },
      { name: 'Alaska', value: 'US-AK' },
      { name: 'Arizona', value: 'US-AZ' },
      { name: 'Arkansas', value: 'US-AR' },
      { name: 'California', value: 'US-CA' },
      { name: 'Colorado', value: 'US-CO' },
      { name: 'Connecticut', value: 'US-CT' },
      { name: 'Delaware', value: 'US-DE' },
      { name: 'Florida', value: 'US-FL' },
      { name: 'Georgia', value: 'US-GA' },
      { name: 'Hawaii', value: 'US-HI' },
      { name: 'Idaho', value: 'US-ID' },
      { name: 'Illinois', value: 'US-IL' },
      { name: 'Indiana', value: 'US-IN' },
      { name: 'Iowa', value: 'US-IA' },
      { name: 'Kansas', value: 'US-KS' },
      { name: 'Kentucky', value: 'US-KY' },
      { name: 'Louisiana', value: 'US-LA' },
      { name: 'Maine', value: 'US-ME' },
      { name: 'Maryland', value: 'US-MD' },
      { name: 'Massachusetts', value: 'US-MA' },
      { name: 'Michigan', value: 'US-MI' },
      { name: 'Minnesota', value: 'US-MN' },
      { name: 'Mississippi', value: 'US-MS' },
      { name: 'Missouri', value: 'US-MO' },
      { name: 'Montana', value: 'US-MT' },
      { name: 'Nebraska', value: 'US-NE' },
      { name: 'Nevada', value: 'US-NV' },
      { name: 'New Hampshire', value: 'US-NH' },
      { name: 'New Jersey', value: 'US-NJ' },
      { name: 'New Mexico', value: 'US-NM' },
      { name: 'New York', value: 'US-NY' },
      { name: 'North Carolina', value: 'US-NC' },
      { name: 'North Dakota', value: 'US-ND' },
      { name: 'Ohio', value: 'US-OH' },
      { name: 'Oklahoma', value: 'US-OK' },
      { name: 'Oregon', value: 'US-OR' },
      { name: 'Pennsylvania', value: 'US-PA' },
      { name: 'Rhode Island', value: 'US-RI' },
      { name: 'South Carolina', value: 'US-SC' },
      { name: 'South Dakota', value: 'US-SD' },
      { name: 'Tennessee', value: 'US-TN' },
      { name: 'Texas', value: 'US-TX' },
      { name: 'Utah', value: 'US-UT' },
      { name: 'Vermont', value: 'US-VT' },
      { name: 'Virginia', value: 'US-VA' },
      { name: 'Washington', value: 'US-WA' },
      { name: 'West Virginia', value: 'US-WV' },
      { name: 'Wisconsin', value: 'US-WI' },
      { name: 'Wyoming', value: 'US-WY' }
    ]
  }

  adsCanadaSubdivisionTargetingCodes_() {
    return [
      { name: loadTimeData.getString('braveRewardsAutoDetectedItem'), value: 'AUTO' },
      { name: loadTimeData.getString('braveRewardsDisabledItem'), value: 'DISABLED' },
      { name: 'Alberta', value: 'CA-AB' },
      { name: 'British Columbia', value: 'CA-BC' },
      { name: 'Manitoba', value: 'CA-MB' },
      { name: 'New Brunswick', value: 'CA-NB' },
      { name: 'Nova Scotia', value: 'CA-NS' },
      { name: 'Ontario', value: 'CA-ON' },
      { name: 'Quebec', value: 'CA-QC' },
      { name: 'Saskatchewan', value: 'CA-SK' }
    ]
  }
}

customElements.define(
  SettingsBraveRewardsPage.is, SettingsBraveRewardsPage)
