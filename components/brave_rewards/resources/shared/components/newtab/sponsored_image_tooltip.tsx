/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { NewTabLink } from '../new_tab_link'
import { BatIcon } from '../icons/bat_icon'
import { CloseIcon } from '../icons/close_icon'
import { TermsOfService } from '../terms_of_service'
import { AsyncButton } from './async_button'

import * as style from './sponsored_image_tooltip.style'

interface Props {
  adsEnabled: boolean
  onEnableAds: () => void
  onClose: () => void
}

export function SponsoredImageTooltip (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function renderLearnMore () {
    return (
      <NewTabLink href='https://brave.com/brave-rewards/'>
        {getString('rewardsLearnMore')}
      </NewTabLink>
    )
  }

  return (
    <style.root>
      <style.close>
        <button onClick={props.onClose}><CloseIcon /></button>
      </style.close>
      <style.body>
        <style.title>
          <BatIcon />{getString('rewardsBraveRewards')}
        </style.title>
        {
          props.adsEnabled
            ? <div>
                {getString('rewardsSponsoredImageEarningText')}&nbsp;
                {renderLearnMore()}
              </div>
            : <div>
                {getString('rewardsSponsoredImageOptInText')}
                <style.action>
                  <AsyncButton onClick={props.onEnableAds}>
                    {getString('rewardsStartUsingRewards')}
                  </AsyncButton>
                </style.action>
                <div>
                  <TermsOfService text={getString('rewardsOptInTerms')} />&nbsp;
                  {renderLearnMore()}
                </div>
              </div>
        }
      </style.body>
    </style.root>
  )
}
