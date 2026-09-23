/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { getString } from './strings'
import { ContentSettingsType } from 'gen/components/content_settings/core/common/content_settings_types.mojom.m'
import { useShieldsApi } from '../api/shields_api_context'
import { DetailsHeader } from './details_header'

import { style } from './fingerprinting_details.style'

const learnMoreUrl =
  'https://support.brave.app/hc/en-us/articles/360022806212-How-do-I-use-Shields-while-browsing#h_01HXSZ8JPHR8YMBEZCT5M0VZTR'

const webcompatSettingNames = new Map(
  Object.entries(ContentSettingsType)
    .filter(([key, value]) => {
      return (
        value > ContentSettingsType.BRAVE_WEBCOMPAT_NONE
        && value < ContentSettingsType.BRAVE_WEBCOMPAT_ALL
      )
    })
    .map(([key, value]) => {
      switch (value) {
        case ContentSettingsType.BRAVE_WEBCOMPAT_AUDIO:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_AUDIO_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_CANVAS:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_CANVAS_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_DEVICE_MEMORY:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_DEVICE_MEMORY_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL:
          return [
            value,
            getString(
              'BRAVE_SHIELDS_BLOCK_FINGERPRINTING_EVENT_SOURCE_POOL_LABEL',
            ),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_FONT:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_FONT_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY:
          return [
            value,
            getString(
              'BRAVE_SHIELDS_BLOCK_FINGERPRINTING_HARDWARE_CONCURRENCY_LABEL',
            ),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_KEYBOARD:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_KEYBOARD_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_LANGUAGE:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_LANGUAGE_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_MEDIA_DEVICES:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_MEDIA_DEVICES_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_PLUGINS:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_PLUGINS_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_SCREEN:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_SCREEN_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS:
          return [
            value,
            getString(
              'BRAVE_SHIELDS_BLOCK_FINGERPRINTING_SPEECH_SYNTHESIS_LABEL',
            ),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER:
          return [
            value,
            getString(
              'BRAVE_SHIELDS_BLOCK_FINGERPRINTING_USB_DEVICE_SERIAL_NUMBER_LABEL',
            ),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_USER_AGENT:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_USER_AGENT_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_WEBGL:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_WEBGL_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_WEBGL2:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_WEBGL2_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_WEBGPU:
          return [
            value,
            getString('BRAVE_SHIELDS_BLOCK_FINGERPRINTING_WEBGPU_LABEL'),
          ]
        case ContentSettingsType.BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL:
          return [
            value,
            getString(
              'BRAVE_SHIELDS_BLOCK_FINGERPRINTING_WEB_SOCKETS_POOL_LABEL',
            ),
          ]
        default: {
          const name = key
            .replace('BRAVE_WEBCOMPAT_', '')
            .replaceAll('_', ' ')
            .toLowerCase()
          return [value, name]
        }
      }
    }),
)

interface Props {
  onBack: () => void
}

export function FingerprintingDetails(props: Props) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: siteSettings } = api.useGetSiteSettings()

  const invokedList = siteBlockInfo?.invokedWebcompatList ?? []
  const webcompatSettings = siteSettings?.webcompatSettings ?? []

  // Build the list of toggles that will be displayed from invokedWebcompatList.
  // Note that the toggle is *on* if the webcompat setting is *off*. (Enabling
  // the webcompat setting disables the fingerprinting protection.)
  const entries = React.useMemo(() => {
    return invokedList
      .map((value) => ({
        name: webcompatSettingNames.get(value) ?? '',
        value,
        enabled: !webcompatSettings[value],
      }))
      .filter((entry) => entry.name)
  }, [invokedList, webcompatSettings])

  if (!siteBlockInfo || !siteSettings) {
    return null
  }

  const enabledCount = entries.filter((entry) => entry.enabled).length

  const title = getString('BRAVE_SHIELDS_FINGERPRINTING_PROTECTIONS_TITLE')

  return (
    <main data-css-scope={style.scope}>
      <DetailsHeader
        title={`${title} (${enabledCount})`}
        onBack={props.onBack}
      >
        <p className='header-text'>
          {getString('BRAVE_SHIELDS_FINGERPRINTING_LIST_DESCRIPTION')}{' '}
          <button onClick={() => api.openTab(learnMoreUrl)}>
            {getString('BRAVE_SHIELDS_LEARN_MORE_LINK_TEXT')}
          </button>
        </p>
      </DetailsHeader>
      <div className='toggle-list scrollable'>
        {entries.map((entry) => (
          <div key={entry.value}>
            {entry.name}
            <Toggle
              size='small'
              checked={entry.enabled}
              onChange={(event) => {
                api.setWebcompatEnabled([entry.value, !event.checked])
              }}
            />
          </div>
        ))}
      </div>
    </main>
  )
}
