/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { BatCrossedIcon } from './icons/bat_crossed_icon'

import * as style from './unsupported_region_notice.style'

export function UnsupportedRegionNotice () {
  const { getString } = React.useContext(LocaleContext)

  return (
    <style.root>
      <BatCrossedIcon />
      <style.heading>
        {getString('unsupportedRegionNoticeHeader')}
      </style.heading>
      <style.content>
        <style.text>
          {getString('unsupportedRegionNoticeSubheader')}
        </style.text>
        <style.text>
          {
            formatMessage(getString('unsupportedRegionNoticeLearnMore'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='learn' href='https://support.brave.com/hc/en-us/articles/9053832354957'>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </style.text>
        <style.spacing />
        <style.text>
          {getString('unsupportedRegionNoticeText1')}
        </style.text>
        <style.text>
          {getString('unsupportedRegionNoticeText2')}
        </style.text>
      </style.content>
    </style.root>
  )
}
