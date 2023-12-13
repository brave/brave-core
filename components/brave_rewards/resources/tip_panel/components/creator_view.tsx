/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { useModelState } from '../lib/model_context'
import { useLocaleContext } from '../lib/locale_strings'
import { formatMessage } from '../../shared/lib/locale_context'
import { lookupPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { LaunchIcon } from './icons/launch_icon'
import { getSocialIcon } from './icons/social_icon'
import { VerifiedTooltip } from './verified_tooltip'

import * as style from './creator_view.style'

export function CreatorView () {
  const { getString } = useLocaleContext()
  const creatorBanner = useModelState(state => state.creatorBanner)
  const creatorVerified = useModelState(state => state.creatorVerified)

  const styleProps = {}

  if (creatorBanner.background) {
    styleProps['--creator-background-image-url'] =
      `url("${creatorBanner.background}")`
  }

  if (creatorBanner.logo) {
    styleProps['--creator-avatar-image-url'] = `url("${creatorBanner.logo}")`
  }

  function getName () {
    if (creatorBanner.title) {
      return creatorBanner.title
    }
    if (creatorBanner.provider) {
      return formatMessage(getString('platformPublisherTitle'), [
        creatorBanner.name,
        lookupPublisherPlatformName(creatorBanner.provider)
      ])
    }
    return creatorBanner.name
  }

  function renderLink (platform: string, url: string) {
    const Icon = getSocialIcon(platform)
    if (!Icon) {
      return null
    }
    return (
      <NewTabLink key={platform} href={url}>
        <Icon />
      </NewTabLink>
    )
  }

  const renderedLinks = Object.entries(creatorBanner.links).map(
    ([key, value]) => renderLink(key, value))

  if (creatorBanner.web3Url) {
    if (renderedLinks.length > 0) {
      renderedLinks.push(<style.linkDivider key='divider' />)
    }
    renderedLinks.push(
      <NewTabLink key='web3' href={creatorBanner.web3Url}>
        <LaunchIcon />
      </NewTabLink>
    )
  }

  return (
    <style.root style={styleProps as React.CSSProperties}>
      <style.background />
      <style.avatar />
      <style.title>
        <style.name>{getName()}</style.name>
        {
          creatorVerified &&
            <style.verifiedCheck>
              <Icon name='verification-filled-color' />
              <div className='tooltip'>
                <VerifiedTooltip />
              </div>
            </style.verifiedCheck>
        }
      </style.title>
      <style.text>
        {creatorBanner.description || getString('defaultCreatorDescription')}
      </style.text>
      {renderedLinks.length > 0 && <style.links>{renderedLinks}</style.links>}
    </style.root>
  )
}
