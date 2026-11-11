// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'
import { App, SLIDES, Slide } from '../components/app'
import { RecapStats, getTimeSavedMs } from '../stats'
import {
  SHARE_CARD_HEIGHT,
  SHARE_CARD_WIDTH,
  ShareCardImages,
  renderShareCard,
} from '../share_card'
import {
  formatBandwidth,
  formatCount,
  formatMonthYear,
  joinValue,
  formatTimeSaved,
} from '../format'
import cardPattern from '../images/card_pattern.png'
import '../components/app.css'

const DAY_MS = 24 * 60 * 60 * 1000
const GB = 1024 * 1024 * 1024

// The page loads these from its own origin; in Storybook they come from the
// bundle and the static icons folder.
const CARD_IMAGES: ShareCardImages = {
  logo: `${location.pathname.replace(/\/[^/]*$/, '')}/icons/brave-icon-release-color.svg`,
  pattern: cardPattern,
}

// Matches the browser frames in the design.
const VIEWPORTS = {
  recapDesktop: {
    name: 'Recap desktop',
    styles: { width: '1366px', height: '940px' },
  },
  recapTablet: {
    name: 'Recap tablet',
    styles: { width: '800px', height: '1172px' },
  },
  recapTabletLandscape: {
    name: 'Recap tablet landscape',
    styles: { width: '1180px', height: '820px' },
  },
  recapPhone: {
    name: 'Recap phone',
    styles: { width: '412px', height: '780px' },
  },
}

interface StoryArgs {
  adsBlocked: number
  dataSavedGB: number
  daysSinceInstall: number
  initialSlide: Slide
}

function getStats(args: StoryArgs): RecapStats {
  return {
    adsBlocked: args.adsBlocked,
    bandwidthSavedBytes: args.dataSavedGB * GB,
    firstRunTime: args.daysSinceInstall
      ? Date.now() - args.daysSinceInstall * DAY_MS
      : 0,
  }
}

function RecapStory(args: StoryArgs) {
  const stats = React.useMemo(
    () => getStats(args),
    [args.adsBlocked, args.dataSavedGB, args.daysSinceInstall],
  )
  return (
    <div
      data-theme='dark'
      style={{ height: '100dvh' }}
    >
      <App
        // Remount so changing a control restarts from the chosen slide.
        key={JSON.stringify(args)}
        stats={stats}
        initialSlide={args.initialSlide}
        cardImages={CARD_IMAGES}
      />
    </div>
  )
}

export default {
  title: 'Brave Recap/Page',
  component: RecapStory,
  parameters: {
    layout: 'fullscreen',
    viewport: { viewports: VIEWPORTS, defaultViewport: 'recapDesktop' },
  },
  args: {
    adsBlocked: 274_184,
    dataSavedGB: 8.83,
    daysSinceInstall: 1573,
    initialSlide: 'intro',
  },
  argTypes: {
    adsBlocked: { control: { type: 'number', min: 0, step: 1000 } },
    dataSavedGB: { control: { type: 'number', min: 0, step: 0.5 } },
    daysSinceInstall: {
      control: { type: 'number', min: 0 },
      description: '0 means the install date is unknown',
    },
    initialSlide: { control: 'select', options: SLIDES },
  },
} satisfies Meta<typeof RecapStory>

type Story = StoryObj<typeof RecapStory>

export const Desktop: Story = {}

export const Tablet: Story = {
  parameters: { viewport: { defaultViewport: 'recapTablet' } },
}

export const TabletLandscape: Story = {
  parameters: { viewport: { defaultViewport: 'recapTabletLandscape' } },
}

export const Phone: Story = {
  parameters: { viewport: { defaultViewport: 'recapPhone' } },
}

export const BlockedSlide: Story = {
  args: { initialSlide: 'blocked' },
}

export const TimeSlide: Story = {
  args: { initialSlide: 'time' },
}

export const DataSlide: Story = {
  args: { initialSlide: 'data' },
}

export const ShareSlide: Story = {
  args: { initialSlide: 'share' },
}

export const HeavyUser: Story = {
  args: {
    adsBlocked: 5_284_213,
    dataSavedGB: 182.4,
    daysSinceInstall: 2900,
    initialSlide: 'blocked',
  },
}

// Below the first time milestone, so the time slide has no comparison.
export const NewUser: Story = {
  args: {
    adsBlocked: 312,
    dataSavedGB: 0.02,
    daysSinceInstall: 2,
  },
}

export const UnknownInstallDate: Story = {
  args: { daysSinceInstall: 0 },
}

export const NotEnoughData: Story = {
  args: { adsBlocked: 12 },
}

// The exported image on its own, at the size it's shared.
function ShareCardStory(args: StoryArgs) {
  const [url, setUrl] = React.useState<string>()
  React.useEffect(() => {
    const stats = getStats(args)
    let objectUrl: string | undefined
    let cancelled = false
    renderShareCard(
      {
        title: 'My Brave recap',
        subtitle: stats.firstRunTime
          ? `Since ${formatMonthYear(stats.firstRunTime)}`
          : '',
        stats: [
          {
            value: formatCount(stats.adsBlocked),
            label: 'ads & trackers blocked',
          },
          {
            value: joinValue(formatTimeSaved(getTimeSavedMs(stats))),
            label: 'of waiting saved',
          },
          {
            value: joinValue(formatBandwidth(stats.bandwidthSavedBytes)),
            label: 'of data saved',
          },
        ],
        cta: 'Get Brave at brave.com',
        url: 'brave://recap',
      },
      CARD_IMAGES,
    ).then((blob) => {
      if (!cancelled) {
        objectUrl = URL.createObjectURL(blob)
        setUrl(objectUrl)
      }
    })
    return () => {
      cancelled = true
      if (objectUrl) {
        URL.revokeObjectURL(objectUrl)
      }
    }
  }, [args.adsBlocked, args.dataSavedGB, args.daysSinceInstall])

  return (
    <div style={{ padding: 24 }}>
      {url && (
        <img
          src={url}
          width={SHARE_CARD_WIDTH / 4}
          height={SHARE_CARD_HEIGHT / 4}
          alt=''
        />
      )}
    </div>
  )
}

export const ShareImage: StoryObj<typeof ShareCardStory> = {
  render: (args) => (
    <ShareCardStory
      adsBlocked={args.adsBlocked}
      dataSavedGB={args.dataSavedGB}
      daysSinceInstall={args.daysSinceInstall}
      initialSlide={args.initialSlide}
    />
  ),
  parameters: { layout: 'centered' },
}
