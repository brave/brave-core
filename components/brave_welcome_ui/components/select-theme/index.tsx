// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as S from './style'

import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import Button from '$web-components/button'

import DataContext from '../../state/context'
import { useViewTypeTransition } from '../../state/hooks'

interface ThemeModeItemProps {
  themeType: chrome.braveTheme.ThemeType
  title: string
  isActive: boolean
  onChange?: (themeType: chrome.braveTheme.ThemeType) => void
}

const themeList = [
  { themeType: 'System', title: getLocale('braveWelcomeSelectThemeSystemLabel') },
  { themeType: 'Light', title: getLocale('braveWelcomeSelectThemeLightLabel') },
  { themeType: 'Dark', title: getLocale('braveWelcomeSelectThemeDarkLabel') }
]

function ThemeModeItem (props: ThemeModeItemProps) {
  const handleClick = () => {
    props.onChange?.(props.themeType)
  }

  const buttonClass = classnames({
    'theme-item': true,
    'is-selected': props.isActive
  })

  const logoBoxClass = classnames({
    'logo': true,
    'dark-mode': props.themeType === 'Dark',
    'system-mode': props.themeType === 'System'
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
      <div className="logo-box">
        <div className={logoBoxClass} />
        <p className="theme-name">{props.title}</p>
      </div>
    </button>
  )
}

function SelectTheme () {
  const { viewType, setViewType, scenes } = React.useContext(DataContext)
  const [currentSelectedTheme, setCurrentTheme] = React.useState<chrome.braveTheme.ThemeType>('System')

  const handleSelectionChange = (themeType: chrome.braveTheme.ThemeType) => {
    setCurrentTheme?.(themeType)
  }

  const { forward } = useViewTypeTransition(viewType)
  const goForward = () => {
    setViewType(forward)
    scenes?.s2.play()
  }

  const handleSkip = () => goForward()
  const handleNext = () => {
    chrome.braveTheme.setBraveThemeType(currentSelectedTheme)
    goForward()
  }

  return (
    <S.Box>
      <div className="view-header-box">
        <div className="view-header-actions">
        </div>
        <div className="view-details">
          <h1 className="view-title">{getLocale('braveWelcomeSelectThemeLabel')}</h1>
        </div>
      </div>
      <S.ThemeListBox>
        <div className={classnames({
          'theme-list': true,
          'is-selected-dark': currentSelectedTheme === 'Dark',
          'is-selected-light': currentSelectedTheme === 'Light'
        })}>
          {themeList.map((entry, id) => {
            return (
              <ThemeModeItem
                key={id}
                themeType={entry.themeType as chrome.braveTheme.ThemeType}
                title={entry.title}
                isActive={entry.themeType === currentSelectedTheme}
                onChange={handleSelectionChange}
              />
            )
          })}
        </div>
      </S.ThemeListBox>
      <div className="view-note">{getLocale('braveWelcomeSelectThemeNote')}</div>
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
          onClick={handleNext}
          scale="jumbo"
        >
          {getLocale('braveWelcomeNextButtonLabel')}
        </Button>
      </S.ActionBox>
    </S.Box>
  )
}

export default SelectTheme
