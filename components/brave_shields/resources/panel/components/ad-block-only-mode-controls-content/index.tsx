// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import Icon, { setIconBasePath } from '@brave/leo/react/icon'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { getLocale } from '$web-common/locale'

import * as S from './style'

setIconBasePath('chrome://resources/brave-icons')

const onSettingsClick = () => {
  chrome.tabs.create({ url: 'chrome://settings/shields', active: true })
}

function IsTheSiteWorkingCorrectlyNowAdBlockOnlyModeNotice() {
  const onLooksGoodClick = () => {
    getPanelBrowserAPI().panelHandler.closeUI()
  }

  const onReportClick = () => {
    getPanelBrowserAPI().dataHandler.openWebCompatWindow()
  }

  return (
    <div style={{ padding: '0 var(--leo-spacing-xl) var(--leo-spacing-xl)' }}>
      <div style={{
        margin: '0 var(--leo-spacing-xl) var(--leo-spacing-xl)',
        display: 'flex',
        alignItems: 'center',
        gap: 'var(--leo-spacing-m)'
      }}>
        <div style={{ display: 'flex', flexDirection: 'column' }}>
          <div style={{
            font: 'var(--leo-font-default-regular)',
            color: 'var(--leo-color-text-primary)',
            marginBottom: 'var(--leo-spacing-xs)'
          }}>
            {getLocale('braveShieldsAdBlockOnlyModeEnabledTitle')}
          </div>
          <div style={{
            font: 'var(--leo-font-small-regular)',
            color: 'var(--leo-color-text-tertiary)'
          }}>
            {getLocale('braveShieldsAdBlockOnlyModeEnabledDesc')}
          </div>
        </div>
        <Button
          kind='plain-faint'
          size='small'
          onClick={onSettingsClick}
        >
          <Icon name='launch' />
        </Button>
      </div>
      <Alert type='info' hideIcon hasActions={false} style={{ paddingTop: 'var(--leo-spacing-m)' }}>
        <div style={{ font: 'var(--leo-font-heading-h4)' }}>
          {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowTitle')}
        </div>
        <div style={{ font: 'var(--leo-font-default-regular)' }}>
          {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowDesc')}
        </div>
        <div
          style={{
            paddingTop: 'var(--leo-spacing-xl)',
            display: 'flex',
            gap: 'var(--leo-spacing-m)'
          }}
        >
          <Button size="medium" onClick={onLooksGoodClick}>{getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowLooksGood')}</Button>
          <Button kind="plain" size="medium" onClick={onReportClick}>{getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowReportSite')}</Button>
        </div>
      </Alert>
    </div>
  )
}

function GlobalSettings() {
  const onAdBlockListsClick = () => {
    chrome.tabs.create({ url: 'chrome://settings/shields/filters', active: true })
  }

  return (
    <S.FooterActionBox>
      <div>
        <button
          onClick={onAdBlockListsClick}
        >
          <i>
            <svg aria-hidden="true" width="18" height="14" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M17.01 2.41H4.26a.709.709 0 0 1 0-1.418h12.75a.709.709 0 0 1 0 1.417ZM.717 11.616h1.416v1.417H.718v-1.417Zm0-3.541h1.416v1.416H.718V8.076Zm0-3.542h1.416v1.417H.718V4.534Zm0-3.542h1.416V2.41H.718V.992Zm3.541 3.542h9.917a.709.709 0 0 1 0 1.417H4.26a.709.709 0 0 1 0-1.417Zm0 3.542h12.75a.709.709 0 0 1 0 1.416H4.26a.709.709 0 0 1 0-1.416Zm0 3.541h9.917a.709.709 0 0 1 0 1.417H4.26a.709.709 0 0 1 0-1.417Z" /></svg>
          </i>
          {getLocale('braveShieldsCustomizeAdblockLists')}
        </button>
        <button
          onClick={onSettingsClick}>
          <i>
            <svg aria-hidden="true" width="18" height="18" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M14.03 11.126a3.191 3.191 0 0 1 3.188 3.186A3.191 3.191 0 0 1 14.03 17.5a3.191 3.191 0 0 1-3.187-3.188 3.191 3.191 0 0 1 3.187-3.186Zm0 1.417c-.227 0-.443.046-.643.125l2.289 2.288c.078-.2.125-.416.125-.644 0-.976-.795-1.77-1.77-1.77Zm0 3.54c.228 0 .444-.046.644-.125l-2.29-2.29c-.078.2-.125.417-.125.644 0 .977.795 1.772 1.771 1.772Z" /><path d="M8.718.5C4.043.5.218 4.325.218 9s3.825 8.5 8.5 8.5h.354a.71.71 0 0 0 .708-.708c0-.355-.354-.638-.708-.638-.708-.92-1.558-2.267-2.054-3.966H8.93c.425 0 .709-.284.709-.709s-.284-.708-.709-.708H6.734c0-.142-.07-.284-.07-.425-.142-1.133-.071-2.054.07-2.975h3.896c.071.637.142 1.346.142 2.054 0 .425.283.708.708.708a.71.71 0 0 0 .709-.708c0-.708 0-1.417-.142-2.125h3.542c.141.496.212 1.063.212 1.63 0 .283 0 .566-.07.85-.072.353.212.708.637.778.354.071.708-.212.779-.637.07-.213.07-.567.07-.921 0-4.675-3.824-8.5-8.5-8.5ZM7.23 2.058c-.566.992-1.275 2.267-1.629 3.896H2.343c.92-1.983 2.762-3.4 4.887-3.896Zm0 13.884c-2.125-.496-3.896-1.913-4.816-3.825H5.6a11.962 11.962 0 0 0 1.63 3.825Zm-1.983-5.525c0 .07 0 .212.07.283h-3.47c-.142-.496-.213-1.133-.213-1.7s.071-1.133.213-1.63h3.471c-.142.922-.213 1.984-.071 3.047Zm3.33-4.463H7.088a12.229 12.229 0 0 1 1.629-3.47c.566.85 1.204 1.983 1.629 3.47h-1.77Zm6.516 0h-3.33c-.424-1.629-1.062-2.975-1.7-3.896 2.196.425 4.109 1.913 5.03 3.896Z" /></svg>
          </i>
          {getLocale('braveShieldsChangeDefaults')}
        </button>
      </div>
    </S.FooterActionBox>
  )
}

function AdBlockOnlyModeControlsContent({ showGlobalSettings }: { showGlobalSettings: boolean }) {
  return (
    <section id='global-controls-content'>
      <IsTheSiteWorkingCorrectlyNowAdBlockOnlyModeNotice />
      {showGlobalSettings && <GlobalSettings />}
    </section>
  )
}

export default AdBlockOnlyModeControlsContent
