/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { IconName } from '@brave/leo/icons/meta'

import { getString } from '../lib/strings'

import { style } from './browser_preview.style'

const pinnedTabIcons: IconName[] = [
  'slack-color',
  'google-calendar-color',
  'gmail-color',
  'social-youtube-color',
]

interface BrowserPreviewProps {
  tabOrientation: 'horizontal' | 'vertical'
}

// Mock browser chrome that illustrates the selected tab layout. Hidden from
// assistive technology.
export function BrowserPreview(props: BrowserPreviewProps) {
  const isVertical = props.tabOrientation === 'vertical'

  return (
    <div
      data-css-scope={style.scope}
      className={props.tabOrientation}
      aria-hidden='true'
    >
      <div className='chrome'>
        <div className='top-bar'>
          {!isVertical && (
            <div className='tab-strip'>
              <div className='pinned-tab'>
                <Icon name='slack-color' />
              </div>
              <ActiveTab />
              <div className='toolbar-button new-tab'>
                <Icon name='plus-add' />
              </div>
              <div className='toolbar-button tab-strip-end'>
                <Icon name='carat-down' />
              </div>
            </div>
          )}
          <AddressBar />
        </div>
        <div className='chrome-body'>
          {isVertical && (
            <div className='tab-strip'>
              <div className='pinned-tabs'>
                {pinnedTabIcons.map((iconName) => (
                  <div
                    key={iconName}
                    className='pinned-tab'
                  >
                    <Icon name={iconName} />
                  </div>
                ))}
              </div>
              <div className='divider' />
              <div className='tabs'>
                <ActiveTab />
              </div>
            </div>
          )}
          <div className='page' />
        </div>
      </div>
    </div>
  )
}

function ActiveTab() {
  return (
    <div className='active-tab'>
      <Icon name='social-brave-release-favicon-fullheight-color' />
      <span className='tab-title'>{getString('WELCOME_PAGE_TITLE')}</span>
      <Icon name='close' />
    </div>
  )
}

function AddressBar() {
  return (
    <div className='address-bar'>
      <div className='nav-buttons'>
        <div className='toolbar-button'>
          <Icon name='browser-back' />
        </div>
        <div className='toolbar-button disabled'>
          <Icon name='browser-forward' />
        </div>
        <div className='toolbar-button'>
          <Icon name='browser-refresh' />
        </div>
        <div className='toolbar-button'>
          <Icon name='browser-bookmark-normal' />
        </div>
      </div>
      <div className='omnibox'>
        <div className='omnibox-button'>
          <Icon name='search' />
        </div>
        <span className='placeholder'>
          {getString('WELCOME_PAGE_PREVIEW_ADDRESS_BAR_TEXT')}
        </span>
        <div className='omnibox-button'>
          <Icon name='social-brave-release-favicon-fullheight-color' />
        </div>
      </div>
      <div className='toolbar-button'>
        <Icon name='hamburger-menu' />
      </div>
    </div>
  )
}
