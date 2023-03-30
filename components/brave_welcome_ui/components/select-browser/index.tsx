// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Button from '$web-components/button'
import classnames from '$web-common/classnames'
import DataContext from '../../state/context'
import { ViewType } from '../../state/component_types'
import { getUniqueBrowserTypes } from '../../state/utils'
import { WelcomeBrowserProxyImpl, ImportDataBrowserProxyImpl, defaultImportTypes } from '../../api/welcome_browser_proxy'
import { getLocale } from '$web-common/locale'

import ChromeCanarySVG from '../svg/browser-icons/chrome-canary'
import ChromeSVG from '../svg/browser-icons/chrome'
import ChromeBetaSVG from '../svg/browser-icons/chrome-beta'
import ChromeDevSVG from '../svg/browser-icons/chrome-dev'
import ChromiumSVG from '../svg/browser-icons/chromium'
import EdgeSVG from '../svg/browser-icons/edge'
import FirefoxSVG from '../svg/browser-icons/firefox'
import OperaSVG from '../svg/browser-icons/opera'
import SafariSVG from '../svg/browser-icons/safari'
import VivaldiSVG from '../svg/browser-icons/vivaldi'
import WhaleSVG from '../svg/browser-icons/whale'
import YandexSVG from '../svg/browser-icons/yandex'
import MicrosoftIE from '../svg/browser-icons/ie'

interface BrowserItemButtonProps {
  browserName: string
  onChange?: (browserName: string) => void
  isActive: boolean
}

const browserIcons = {
  'Google Chrome Canary': <ChromeCanarySVG />,
  'Google Chrome': <ChromeSVG />,
  'Google Chrome Dev': <ChromeDevSVG />,
  'Google Chrome Beta': <ChromeBetaSVG />,
  'Chromium': <ChromiumSVG />,
  'Microsoft Edge': <EdgeSVG />,
  'Firefox': <FirefoxSVG />,
  'Opera': <OperaSVG />,
  'Safari': <SafariSVG />,
  'Vivaldi': <VivaldiSVG />,
  'NAVER Whale': <WhaleSVG />,
  'Yandex': <YandexSVG />,
  'Microsoft Internet Explorer': <MicrosoftIE />
}

function BrowserItemButton (props: BrowserItemButtonProps) {
  const handleClick = () => {
    props.onChange?.(props.browserName)
  }

  const buttonClass = classnames({
    'browser-item': true,
    'is-selected': props.isActive
  })

  return (
    <button onClick={handleClick}
      className={buttonClass}
    >
      <i className="check-icon-box">
        {props.isActive && (
          <svg viewBox="0 0 16 12" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M14.9558.9327c-.2259-.2134-.5083-.2667-.7907-.2667s-.5648.16-.7907.3733l-7.06 7.68-3.7276-3.4666c-.4518-.4267-1.186-.4267-1.5814 0-.226.2133-.3389.48-.3389.7466 0 .2667.113.5334.3389.7467l4.5748 4.2667c.1695.2133.4519.32.7907.32h.0565c.3389 0 .6213-.16.7907-.3734l7.8507-8.5333c.3953-.4267.3389-1.12-.113-1.4933Z" fill="#4C54D2"/>
          </svg>
        )}
      </i>
      <div className="browser-logo-box">
        {browserIcons[props.browserName]}
      </div>
      <p className="browser-name">{props.browserName}</p>
    </button>
  )
}

function SelectBrowser () {
  const { browserProfiles, currentSelectedBrowser, setCurrentSelectedBrowser, setViewType, incrementCount, scenes } = React.useContext(DataContext)
  const browserTypes = getUniqueBrowserTypes(browserProfiles ?? [])
  const handleSelectionChange = (browserName: string) => {
    setCurrentSelectedBrowser?.(browserName)
  }

  // TODO(tali): we're duping this call in SelectProfile component.
  // Perhaps compute this at root component
  const filteredProfiles = browserProfiles?.filter(
    profile => profile.browserType === currentSelectedBrowser)

  const handleImport = () => {
    if (!currentSelectedBrowser || !filteredProfiles) return
    if (filteredProfiles.length > 1) {
      // If there are multiple profiles, we handle it in a different view
      setViewType(ViewType.ImportSelectProfile)
    } else {
      ImportDataBrowserProxyImpl.getInstance().importData(filteredProfiles[0].index, defaultImportTypes)
      incrementCount()
    }

    WelcomeBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.ImportSelectBrowser, isFinished: false, isSkipped: false })
  }

  const handleSkip = () => {
    scenes?.s2.play() // play the final animation on skip
    setViewType(ViewType.HelpImprove)
    WelcomeBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.ImportSelectBrowser, isFinished: false, isSkipped: true })
  }

  React.useEffect(() => {
    WelcomeBrowserProxyImpl.getInstance().getDefaultBrowser().then(
      (name: string) => {
      setCurrentSelectedBrowser?.(name)
    })
  }, [])

  const hasSelectedBrowser = filteredProfiles && filteredProfiles.length > 0

  return (
    <S.MainBox>
      <div className="view-header-box">
        <div className="view-details">
          <h1 className="view-title">{getLocale('braveWelcomeImportSettingsTitle')}</h1>
          <p className="view-desc">{getLocale('braveWelcomeImportSettingsDesc')}</p>
        </div>
      </div>
      <S.BrowserListBox>
        <div className="browser-list">
          {browserTypes.map((entry, id) => {
            return (
              <BrowserItemButton
                key={id}
                browserName={entry ?? 'Chromium-based browser'}
                onChange={handleSelectionChange}
                isActive={entry === currentSelectedBrowser}
              />
            )
          })}
        </div>
      </S.BrowserListBox>
      <S.ActionBox>
        <Button
          isTertiary={true}
          onClick={handleSkip}
          scale="jumbo"
        >
          {getLocale('braveWelcomeSkipButtonLabel')}
        </Button>
          <Button
            isPrimary={true}
            isDisabled={!hasSelectedBrowser}
            onClick={handleImport}
            scale="jumbo"
          >
            {getLocale('braveWelcomeImportButtonLabel')}
          </Button>
      </S.ActionBox>
    </S.MainBox>
  )
}

export default SelectBrowser
