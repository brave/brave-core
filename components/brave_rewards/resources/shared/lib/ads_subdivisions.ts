/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'

export function getAdsSubdivisions (
  rewardsData: Rewards.State
): Array<[string, string]> {
  const {
    currentCountryCode
  } = rewardsData

  const {
    adsSubdivisionTargeting,
    automaticallyDetectedAdsSubdivisionTargeting
  } = rewardsData.adsData

  let subdivisions: Array<[string, string]> = []
  if (currentCountryCode === 'US') {
    subdivisions = getUnitedStatesSubdivisions()
  } else if (currentCountryCode === 'CA') {
    subdivisions = getCanadaSubdivisions()
  } else {
    return subdivisions
  }

  if (adsSubdivisionTargeting === 'DISABLED') {
    subdivisions.unshift(['DISABLED', getLocale('adsSubdivisionTargetingDisabled')])
  } else {
    subdivisions.unshift(['DISABLED', getLocale('adsSubdivisionTargetingDisable')])
  }

  const subdivisionMap = new Map<string, string>(subdivisions)
  const subdivision = subdivisionMap.get(automaticallyDetectedAdsSubdivisionTargeting)
  if (subdivision && adsSubdivisionTargeting === 'AUTO') {
    subdivisions.unshift(['AUTO', getLocale('adsSubdivisionTargetingAutoDetectedAs', { adsSubdivisionTarget: subdivision })])
  } else {
    subdivisions.unshift(['AUTO', getLocale('adsSubdivisionTargetingAutoDetect')])
  }

  return subdivisions
}

function getUnitedStatesSubdivisions (): Array<[string, string]> {
  return [
    ['US-AL', 'Alabama'],
    ['US-AK', 'Alaska'],
    ['US-AZ', 'Arizona'],
    ['US-AR', 'Arkansas'],
    ['US-CA', 'California'],
    ['US-CO', 'Colorado'],
    ['US-CT', 'Connecticut'],
    ['US-DE', 'Delaware'],
    ['US-FL', 'Florida'],
    ['US-GA', 'Georgia'],
    ['US-HI', 'Hawaii'],
    ['US-ID', 'Idaho'],
    ['US-IL', 'Illinois'],
    ['US-IN', 'Indiana'],
    ['US-IA', 'Iowa'],
    ['US-KS', 'Kansas'],
    ['US-KY', 'Kentucky'],
    ['US-LA', 'Louisiana'],
    ['US-ME', 'Maine'],
    ['US-MD', 'Maryland'],
    ['US-MA', 'Massachusetts'],
    ['US-MI', 'Michigan'],
    ['US-MN', 'Minnesota'],
    ['US-MS', 'Mississippi'],
    ['US-MO', 'Missouri'],
    ['US-MT', 'Montana'],
    ['US-NE', 'Nebraska'],
    ['US-NV', 'Nevada'],
    ['US-NH', 'New Hampshire'],
    ['US-NJ', 'New Jersey'],
    ['US-NM', 'New Mexico'],
    ['US-NY', 'New York'],
    ['US-NC', 'North Carolina'],
    ['US-ND', 'North Dakota'],
    ['US-OH', 'Ohio'],
    ['US-OK', 'Oklahoma'],
    ['US-OR', 'Oregon'],
    ['US-PA', 'Pennsylvania'],
    ['US-RI', 'Rhode Island'],
    ['US-SC', 'South Carolina'],
    ['US-SD', 'South Dakota'],
    ['US-TN', 'Tennessee'],
    ['US-TX', 'Texas'],
    ['US-UT', 'Utah'],
    ['US-VT', 'Vermont'],
    ['US-VA', 'Virginia'],
    ['US-WA', 'Washington'],
    ['US-WV', 'West Virginia'],
    ['US-WI', 'Wisconsin'],
    ['US-WY', 'Wyoming']
  ]
}

function getCanadaSubdivisions (): Array<[string, string]> {
  return [
    ['CA-AB', 'Alberta'],
    ['CA-BC', 'British Columbia'],
    ['CA-MB', 'Manitoba'],
    ['CA-NB', 'New Brunswick'],
    ['CA-NS', 'Nova Scotia'],
    ['CA-ON', 'Ontario'],
    ['CA-QC', 'Quebec'],
    ['CA-SK', 'Saskatchewan']
  ]
}
