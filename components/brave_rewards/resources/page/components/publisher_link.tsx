/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { lookupPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { VerifiedCheckIcon } from './icons/verified_check_icon'

import * as style from './publisher_link.style'

interface Props {
  name: string
  url: string
  icon: string
  platform: string
  verified: boolean
}

export function PublisherLink (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const iconPath = props.verified && props.icon || props.url
  const platformName = lookupPublisherPlatformName(props.platform)

  return (
    <style.root>
      <NewTabLink href={props.url}>
        <style.icon>
          <img src={`chrome://favicon/size/64@1x/${iconPath}`} />
          {
            props.verified &&
              <style.verified>
                <VerifiedCheckIcon />
              </style.verified>
          }
        </style.icon>
        {props.name}
        {
          platformName &&
            <style.platform>
              &nbsp;{getString('on')}&nbsp;{platformName}
            </style.platform>
        }
      </NewTabLink>
    </style.root>
  )
}
