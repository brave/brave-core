// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import AutoSizer from '@brave/react-virtualized-auto-sizer'
import { VariableSizeList } from 'react-window'
import getBraveNewsController, { Channels, Publisher, UserEnabled } from '../../../../api/brave_news'
import {
  SettingsRow,
  SettingsText
} from '../../../../components/default'
import { Toggle } from '../../../../components/toggle'
import * as s from './style'
import usePromise from '../../../../hooks/usePromise'
import { defaultState as newTabData } from '../../../../storage/new_tab_storage'

/**
 * Determines whether a publishers content is shown in the feed. This might mean
 * it belongs to an active channel, or that the user has explicitly turned it on
 * or off, or that the feed is enabled by default (if channels are not available).
 * @param publisher The publisher to check.
 * @param channels All the current channels, with up to date subscriptions
 * @returns Whether the publisher is current enabled.
 */
function isPublisherContentAllowed (publisher: Publisher, channels: Channels): boolean {
  // If the user has an explicit preference for this feed, use that.
  if (publisher.userEnabledStatus === UserEnabled.ENABLED) return true
  if (publisher.userEnabledStatus === UserEnabled.DISABLED) return false

  // If we're using the old API, check if the source is default enabled.
  if (!newTabData.featureFlagBraveNewsV2Enabled) return publisher.isEnabled

  // Otherwise, we're using the channels API - the publisher is allowed if it's
  // in any of the channels we're subscribed to, in any of the locales the
  // channel is available in.
  for (const localeInfo of publisher.locales) {
    for (const channel of localeInfo.channels) {
      if (channels[channel].subscribedLocales.includes(localeInfo.locale)) {
        return true
      }
    }
  }

  return false
}

/**
 * Depending on whether we're using the V2 version of Brave News, either return
 * the list of channels or an array containing the categoryName, for backwards
 * compatibility.
 *
 * Once we've turned on the new sources.json for everyone, we can simply remove
 * this function and always use channels.
 * @param publisher The publisher to get channels for.
 */
export function getPublisherChannels (publisher: Publisher) {
  const allChannels = new Set<string>()
  for (const localeInfo of publisher.locales) {
    for (const channel of localeInfo.channels) {
      allChannels.add(channel)
    }
  }

  return newTabData.featureFlagBraveNewsV2Enabled ? Array.from(allChannels) : [publisher.categoryName]
}

export const DynamicListContext = React.createContext<
  Partial<{ setSize: (index: number, size: number) => void }>
>({})

type PublisherPrefsProps = {
  setPublisherPref: (publisherId: string, enabled: boolean) => any
  publishers: Publisher[]
}

type ListItemProps = {
  index: number
  width: number
  data: Publisher[]
  style: React.CSSProperties
  channels: Channels
  setPublisherPref: (publisherId: string, enabled: boolean) => any
}

// Component for each item. Measures height to let virtual
// list know.
function ListItem (props: ListItemProps) {
  const { setSize } = React.useContext(DynamicListContext)
  const rowRoot = React.useRef<null | HTMLDivElement>(null)

  React.useEffect(() => {
    if (rowRoot.current) {
      const marginBottom = parseFloat(getComputedStyle(rowRoot.current).marginBottom || '0')

      setSize && setSize(props.index, rowRoot.current.getBoundingClientRect().height + marginBottom)
    }
  }, [props.index, setSize, props.width])

  const publisher = props.data[props.index]
  const isChecked = publisher && isPublisherContentAllowed(publisher, props.channels)

  const onChange = React.useCallback(() => {
    if (!publisher) {
      return
    }
    props.setPublisherPref(publisher.publisherId, !isChecked)
  }, [publisher, isChecked])

  if (!publisher) {
    console.warn('Publisher was null at index', props.index, props.data, props)
    return null
  }

  return (
    <s.PublisherListItem style={props.style}>
      <SettingsRow ref={rowRoot} key={publisher.publisherId} onClick={onChange} isInteractive={true}>
        <SettingsText>{publisher.publisherName}</SettingsText>
        <Toggle
          checked={isChecked}
          onChange={onChange}
          size='large'
        />
      </SettingsRow>
    </s.PublisherListItem>
  )
}

// TODO: When we can subscribe to channels, make sure we use the most up to date
// channels. For now though, this is fine.
const channelsPromise = getBraveNewsController().getChannels().then(r => r.channels as Channels)

export default function PublisherPrefs (props: PublisherPrefsProps) {
  const listRef = React.useRef<VariableSizeList | null>(null)
  const sizeMap = React.useRef<{ [key: string]: number }>({})
  const { result: channels = {} } = usePromise(() => channelsPromise, [])

  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (sizeMap.current[index] !== size) {
      sizeMap.current = { ...sizeMap.current, [index]: size }
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [])

  const getSize = React.useCallback((index) => {
    return sizeMap.current[index] || 900
  }, [])

  const publishers = props.publishers

  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <AutoSizer>
        {function ({ width, height }) {
          return (
            <VariableSizeList
              ref={listRef}
              width={width}
              height={height}
              itemData={publishers}
              itemCount={publishers.length}
              itemSize={getSize}
              overscanCount={38}
              style={{
                overscrollBehavior: 'contain'
              }}
            >
              {function (itemProps) {
                return (
                  <ListItem
                    {...itemProps}
                    width={width}
                    channels={channels}
                    setPublisherPref={props.setPublisherPref}
                  />
                )
              }}
            </VariableSizeList>
          )
        }}
      </AutoSizer>
    </DynamicListContext.Provider>
  )
}
