/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  AlertCircleIcon,
  RedditColorIcon,
  TwitterColorIcon,
  TwitchColorIcon,
  GitHubColorIcon,
  YoutubeColorIcon,
  VimeoColorIcon
} from 'brave-ui/components/icons'

import {
  PublisherInfo,
  PublisherStatus,
  ExternalWalletInfo,
  ExternalWalletStatus,
  MediaMetaData,
  BalanceInfo
} from '../lib/interfaces'

import { HostContext } from '../lib/host_context'
import { Locale, LocaleContext } from '../../shared/lib/locale_context'
import { formatLocaleTemplate } from '../lib/formatting'

import { MediaCard } from './media_card'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { VerifiedIcon } from './icons/verified_icon'

import * as style from './publisher_banner.style'

function getLogoURL (publisherInfo: PublisherInfo) {
  const { logo } = publisherInfo
  if (!logo || publisherInfo.status === PublisherStatus.NOT_VERIFIED) {
    return ''
  }
  if (/^https:\/\/[a-z0-9-]+\.invalid(\/)?$/.test(logo)) {
    return `chrome://favicon/size/160@1x/${logo}`
  }
  return logo
}

function getLogo (publisherInfo: PublisherInfo) {
  const logoURL = getLogoURL(publisherInfo)
  if (logoURL) {
    return <img src={logoURL} />
  }
  const name = publisherInfo.name || publisherInfo.publisherKey
  return (
    <style.logoLetter>
      {name ? name[0] : ''}
    </style.logoLetter>
  )
}

function getProviderName (publisherInfo: PublisherInfo) {
  switch (publisherInfo.provider) {
    case 'youtube': return 'YouTube'
    case 'twitter': return 'Twitter'
    case 'twitch': return 'Twitch'
    case 'reddit': return 'Reddit'
    case 'vimeo': return 'Vimeo'
    case 'github': return 'GitHub'
    default: return ''
  }
}

function getSocialScreenName (mediaMetaData: MediaMetaData) {
  switch (mediaMetaData.mediaType) {
    case 'twitter':
      return '@' + mediaMetaData.publisherName
    case 'github':
      return '@' + mediaMetaData.publisherScreenName
    case 'reddit':
      return 'u/' + mediaMetaData.publisherName
  }
  return ''
}

function getPublisherName (
  locale: Locale,
  publisherInfo: PublisherInfo,
  mediaMetaData: MediaMetaData
) {
  const name = publisherInfo.name || publisherInfo.publisherKey
  const platform = getProviderName(publisherInfo)
  if (!platform) {
    return name
  }
  const screenName = getSocialScreenName(mediaMetaData)
  return (
    <>
      {name} {locale.getString('on')} {platform}
      {screenName && <style.socialName>{screenName}</style.socialName>}
    </>
  )
}

function getSocialIcon (type: string) {
  switch (type) {
    case 'twitter': return <TwitterColorIcon />
    case 'youtube': return <YoutubeColorIcon />
    case 'twitch': return <TwitchColorIcon />
    case 'github': return <GitHubColorIcon />
    case 'reddit': return <RedditColorIcon />
    case 'vimeo': return <VimeoColorIcon />
    default: return null
  }
}

function isValidSocialLink (url: string) {
  try {
    // The URL constructor will throw when provided with any
    // string that is not an absolute URL. If the URL constuctor
    // does not throw, consider it a valid social link URL.
    // tslint:disable-next-line:no-unused-expression
    new URL(url)
    return true
  } catch (_) {
    return false
  }
}

function getSocialLinks (publisherInfo: PublisherInfo) {
  return Object.entries(publisherInfo.links).map(([type, url]) => {
    const icon = getSocialIcon(type)
    return icon && isValidSocialLink(url)
      ? <NewTabLink key={type} href={url}>{icon}</NewTabLink>
      : null
  })
}

function getTitle (
  locale: Locale,
  publisherInfo: PublisherInfo,
  mediaMetaData: MediaMetaData
) {
  // For Twitter and Reddit posts a media card is displayed
  // instead of a title
  if (mediaMetaData.mediaType === 'twitter' ||
      mediaMetaData.mediaType === 'reddit') {
    return
  }

  return publisherInfo.title || locale.getString('welcome')
}

function getVerifiedIcon (publisherInfo: PublisherInfo) {
  if (publisherInfo.status === PublisherStatus.NOT_VERIFIED) {
    return null
  }
  return <VerifiedIcon />
}

function showUnverifiedNotice (
  publisherInfo: PublisherInfo,
  balanceInfo?: BalanceInfo,
  externalWalletInfo?: ExternalWalletInfo
) {
  // Show the notice if the publisher is not registered
  if (publisherInfo.status === PublisherStatus.NOT_VERIFIED) {
    return true
  }

  // If the user does not have a connected wallet, do not show the notice
  if (!externalWalletInfo) {
    return false
  }
  switch (externalWalletInfo.status) {
    case ExternalWalletStatus.DISCONNECTED_NOT_VERIFIED:
    case ExternalWalletStatus.DISCONNECTED_VERIFIED:
    case ExternalWalletStatus.NOT_CONNECTED:
      return false
  }

  // Show the notice if the publisher is verified and their wallet provider does
  // not match the user's external wallet provider
  switch (publisherInfo.status) {
    case PublisherStatus.UPHOLD_VERIFIED:
      return externalWalletInfo.type !== 'uphold'
    case PublisherStatus.BITFLYER_VERIFIED:
      return externalWalletInfo.type !== 'bitflyer'
  }

  // Show the notice if the user does not have any brave funds
  const hasBraveFunds = Boolean(balanceInfo && (
    balanceInfo.wallets['anonymous'] ||
    balanceInfo.wallets['blinded']))

  return !hasBraveFunds
}

function getUnverifiedNotice (
  locale: Locale,
  publisherInfo: PublisherInfo,
  balanceInfo: BalanceInfo | undefined,
  walletInfo: ExternalWalletInfo | undefined
) {
  if (!showUnverifiedNotice(publisherInfo, balanceInfo, walletInfo)) {
    return null
  }

  const { getString } = locale

  const text = getString(publisherInfo.status === PublisherStatus.CONNECTED
    ? 'siteBannerConnectedText'
    : 'siteBannerNoticeText')

  return (
    <style.unverifiedNotice>
      <style.unverifiedNoticeIcon>
        <AlertCircleIcon />
      </style.unverifiedNoticeIcon>
      <style.unverifiedNoticeText>
        <strong>{getString('siteBannerNoticeNote')}</strong>&nbsp;
        {text}&nbsp;
        <NewTabLink href='https://brave.com/faq/#unclaimed-funds'>
          {getString('unverifiedTextMore')}
        </NewTabLink>
      </style.unverifiedNoticeText>
    </style.unverifiedNotice>
  )
}

function getPostRelativeTime (postDate: Date) {
  // TS does not yet recongnize RelativeTimeFormatter (since chromium 71)
  const { RelativeTimeFormat } = Intl as any
  const formatter = new RelativeTimeFormat()
  const sec = Math.max(0, Date.now() - postDate.getTime()) / 1000
  if (sec < 60) {
    return formatter.format(-Math.round(sec), 'second')
  }
  if (sec < 60 * 60) {
    return formatter.format(-Math.round(sec / 60), 'minute')
  }
  if (sec < 60 * 60 * 24) {
    return formatter.format(-Math.round(sec / 60 / 60), 'hour')
  }
  return ''
}

function getPostTimeString (postTimestamp: string) {
  const postDate = new Date(postTimestamp)
  const invalidDate = !Number(postDate)
  if (invalidDate) {
    return postTimestamp
  }
  const relative = getPostRelativeTime(postDate)
  if (relative) {
    return relative
  }
  return postDate.toLocaleDateString(undefined, {
    month: 'short',
    day: 'numeric',
    year: postDate.getFullYear() !== new Date().getFullYear()
      ? 'numeric'
      : undefined
  })
}

function getDescription (
  locale: Locale,
  publisherInfo: PublisherInfo,
  mediaMetaData: MediaMetaData
) {
  const { getString } = locale

  if (mediaMetaData.mediaType === 'twitter') {
    const postTime = getPostTimeString(mediaMetaData.postTimestamp)
    const title = formatLocaleTemplate(getString('postHeaderTwitter'), {
      user: publisherInfo.name
    })
    return (
      <MediaCard title={title} postTime={postTime} icon={<TwitterColorIcon />}>
        {mediaMetaData.postText}
      </MediaCard>
    )
  }

  if (mediaMetaData.mediaType === 'reddit') {
    const postTime = getPostTimeString(mediaMetaData.postTimestamp)
    const title = formatLocaleTemplate(getString('postHeader'), {
      user: publisherInfo.name
    })
    return (
      <MediaCard title={title} postTime={postTime} icon={<RedditColorIcon />}>
        {mediaMetaData.postText}
      </MediaCard>
    )
  }

  if (publisherInfo.description) {
    return publisherInfo.description
  }

  return getString('rewardsBannerText1')
}

function hashString (input: string) {
  let h = 0
  for (let i = 0; i < input.length; i++) {
    h = (((h << 5) - h) + input.charCodeAt(i)) >>> 0
  }
  return h
}

function getBackgroundClass (publisherInfo: PublisherInfo) {
  if (publisherInfo.background) {
    return 'background-style-custom'
  }

  // Vary the background type by publisher key and day
  const hash = hashString(
    publisherInfo.publisherKey + '-' +
    Math.floor(Date.now() / 1000 / 60 / 60 / 24))

  return `background-style-${String(hash % 5 + 1)}`
}

export function PublisherBanner () {
  const host = React.useContext(HostContext)
  const locale = React.useContext(LocaleContext)

  const [publisherInfo, setPublisherInfo] = React.useState(
    host.state.publisherInfo)
  const [balanceInfo, setBalanceInfo] = React.useState(
    host.state.balanceInfo)
  const [walletInfo, setWalletInfo] = React.useState(
    host.state.externalWalletInfo)

  React.useEffect(() => {
    return host.addListener((state) => {
      setPublisherInfo(state.publisherInfo)
      setBalanceInfo(state.balanceInfo)
      setWalletInfo(state.externalWalletInfo)
    })
  }, [host])

  if (!publisherInfo) {
    return <style.loading />
  }

  const { mediaMetaData } = host.getDialogArgs()

  const backgroundImage = publisherInfo.background
    ? `url(${publisherInfo.background})`
    : undefined

  const verifiedType = publisherInfo.status === PublisherStatus.NOT_VERIFIED
    ? 'not-verified'
    : 'verified'

  return (
    <style.root style={{ backgroundImage }}>
      <style.background className={getBackgroundClass(publisherInfo)}>
        <style.card>
          <style.header>
            <style.logo>
              <style.logoMask>
                {getLogo(publisherInfo)}
              </style.logoMask>
              <style.verifiedIcon className={verifiedType}>
                {getVerifiedIcon(publisherInfo)}
              </style.verifiedIcon>
            </style.logo>
            <style.name>
              {getPublisherName(locale, publisherInfo, mediaMetaData)}
            </style.name>
          </style.header>
          <style.socialLinks>
            {getSocialLinks(publisherInfo)}
          </style.socialLinks>
          {getUnverifiedNotice(locale, publisherInfo, balanceInfo, walletInfo)}
          <style.title>
            {getTitle(locale, publisherInfo, mediaMetaData)}
          </style.title>
          <style.description>
            {getDescription(locale, publisherInfo, mediaMetaData)}
          </style.description>
        </style.card>
      </style.background>
    </style.root>
  )
}
