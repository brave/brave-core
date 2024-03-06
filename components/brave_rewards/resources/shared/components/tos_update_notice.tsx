/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '@brave/leo/react/button'

import { LocaleContext, formatMessage } from '../lib/locale_context'
import { NewTabLink } from './new_tab_link'

import * as urls from '../lib/rewards_urls'
import * as style from './tos_update_notice.style'

interface Props {
  onAccept: () => void
  onResetRewards: () => void
}

export function TosUpdateNotice (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.heading>
        {getString('rewardsTosUpdateHeading')}
      </style.heading>
      <style.text>
        {
          formatMessage(getString('rewardsTosUpdateText'), {
            tags: {
              $1: (content) => (
                <button
                  key='reset'
                  className='rewards-tos-update-reset-button'
                  onClick={props.onResetRewards}
                >
                  {content}
                </button>
              )
            }
          })
        }
      </style.text>
      <style.text>
        {
          formatMessage(getString('rewardsTosUpdateLinkText'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='link' href={urls.termsOfServiceURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </style.text>
      <Button
        className='rewards-tos-update-accept-button'
        onClick={props.onAccept}
      >
        {getString('rewardsTosUpdateButtonLabel')}
      </Button>
    </style.root>
  )
}
