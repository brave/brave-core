/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { InfoCardProps } from '../../../../src/features/rewards/infoCards'
import { WelcomePage, WelcomePageImages } from '../../../../src/features/rewards/welcomePage'

// Assets
import { getLocale } from '../../../../src/helpers'

// Images
const batLogo = require('../../../assets/img/bat.svg')
const downArrow = require('../../../assets/img/downArrow.svg')
const turnOnRewardsImage = require('../../../assets/img/turnOnRewards.svg')
const braveAdsImage = require('../../../assets/img/braveAds.svg')
const braveContributeImage = require('../../../assets/img/braveContribute.svg')

export interface RewardsWelcomeProps {
  optInAction: () => void
}

class RewardsWelcome extends React.PureComponent<RewardsWelcomeProps, {}> {

  get pageImages (): WelcomePageImages {
    return {
      batLogo,
      downArrow
    }
  }

  get infoItems (): InfoCardProps[] {
    return [
      {
        title: getLocale('turnOnRewardsTitle'),
        description: getLocale('turnOnRewardsDesc'),
        icon: turnOnRewardsImage
      },
      {
        title: getLocale('braveAdsTitle'),
        description: getLocale('braveAdsDesc'),
        icon: braveAdsImage
      },
      {
        title: getLocale('braveContributeTitle'),
        description: getLocale('braveContributeDesc'),
        icon: braveContributeImage
      }
    ]
  }

  render () {
    const { optInAction } = this.props

    return (
      <WelcomePage
        id={'welcome-page'}
        optInAction={optInAction}
        infoItems={this.infoItems}
        pageImages={this.pageImages}
      />
    )
  }
}

export default RewardsWelcome
