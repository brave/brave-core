/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { NewTabLink } from '../new_tab_link'

import * as style from './unsupported_region_card.style'

export function UnsupportedRegionCard () {
  const { getString } = React.useContext(LocaleContext)

  return (
    <style.root>
      <style.heading>
        {getString('rewardsUnsupportedRegionNoticeHeader')}
      </style.heading>
      <style.content>
        <span>
          {getString('rewardsUnsupportedRegionNoticeSubheader')}&nbsp;
          <style.link>
            {
              formatMessage(getString('rewardsUnsupportedRegionNoticeLearnMore'), {
                tags: {
                  $1: (content) => (
                    <NewTabLink key='learn' href='https://support.brave.com/hc/en-us/articles/9053832354957'>
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </style.link>
        </span>
        <style.spacing />
        <style.text>
          {getString('rewardsUnsupportedRegionNoticeText1')}
        </style.text>
        <style.text>
          {getString('rewardsUnsupportedRegionNoticeText2')}
        </style.text>
      </style.content>
    </style.root>
  )
}
