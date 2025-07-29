/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import type { Meta, StoryObj } from '@storybook/react'

import './storybook_locale'

import { NewTabProvider } from '../context/new_tab_context'
import { BackgroundProvider } from '../context/background_context'
import { TopSitesProvider } from '../context/top_sites_context'
import { FakeAIChatContext } from '../context/fake_ai_chat_context'

import { createNewTabHandler } from './new_tab_handler'
import { createBackgroundHandler } from './background_handler'
import { createTopSitesHandler } from './top_sites_handler'

import { StorybookArgs } from './storybook_args'

import { AskApp } from '../components/ask/ask_app'

function StorybookApp(props: StorybookArgs) {
  return (
    <NewTabProvider createHandler={createNewTabHandler}>
      <BackgroundProvider
        createHandler={(s) => createBackgroundHandler(s, props)}
      >
        <TopSitesProvider createHandler={createTopSitesHandler}>
          <FakeAIChatContext>
            <div style={{ position: 'absolute', inset: 0 }}>
              <AskApp />
            </div>
          </FakeAIChatContext>
        </TopSitesProvider>
      </BackgroundProvider>
    </NewTabProvider>
  )
}

export default {
  title: 'New Tab Page/Ask',
  component: StorybookApp,
} satisfies Meta<typeof StorybookApp>

export const AskNewTab: StoryObj<typeof StorybookApp> = {}
