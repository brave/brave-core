/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { getString } from '../lib/strings'
import { ContentSettingsType } from '../lib/shields_store'
import { useShieldsState, useShieldsActions } from '../lib/shields_context'
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
      const name = key
        .replace('BRAVE_WEBCOMPAT_', '')
        .replaceAll('_', ' ')
        .toLowerCase()
      return [value, name]
    }),
)

interface Props {
  onBack: () => void
}

export function FingerprintingDetails(props: Props) {
  const actions = useShieldsActions()
  const invokedList = useShieldsState(
    (s) => s.siteBlockInfo.invokedWebcompatList,
  )
  const webcompatSettings = useShieldsState(
    (s) => s.siteSettings.webcompatSettings,
  )

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

  const enabledCount = entries.filter((entry) => entry.enabled).length

  const title = getString('BRAVE_SHIELDS_FINGERPRINTING_PROTECTIONS_TITLE')

  return (
    <main data-css-scope={style.scope}>
      <DetailsHeader
        title={`${title} (${enabledCount})`}
        onBack={props.onBack}
      >
        <p className='header-text'>
          {getString('BRAVE_SHIELDS_FINGERPRINTING_LIST_DESCRIPTION')}
          <button onClick={() => actions.openTab(learnMoreUrl)}>
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
                actions.setWebcompatEnabled(entry.value, !event.checked)
              }}
            />
          </div>
        ))}
      </div>
    </main>
  )
}
