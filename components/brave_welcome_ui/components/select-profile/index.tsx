// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Button from '$web-components/button'
import LeftArrowSVG from '../svg/left-arrow'
import AvatarIconSVG from '../svg/avatar-icon'
import DataContext from '../../state/context'
import { ViewType } from '../../state/component_types'
import { WelcomeBrowserProxyImpl, ImportDataBrowserProxyImpl, defaultImportTypes } from '../../api/welcome_browser_proxy'
import { getLocale } from '$web-common/locale'

interface ProfileItemProps {
  onChange?: (e: React.ChangeEvent<HTMLInputElement>) => void
  profileName: string
  id: number
  isChecked: boolean
}

function ProfileItem (props: ProfileItemProps) {
  return (
    <div className="item-box">
      <label className="item-grid">
        <div className="item-info">
          <div className="avatar">
            <AvatarIconSVG />
          </div>
          <span>{props.profileName}</span>
        </div>
        <div className="item-action">
          <input
            type="checkbox"
            id={`profile-${props.id}`}
            checked={props.isChecked}
            onChange={props.onChange}
          />
        </div>
      </label>
    </div>
  )
}

function SelectProfile () {
  const { browserProfiles, currentSelectedBrowser, setViewType, incrementCount } = React.useContext(DataContext)
  const filteredProfiles = browserProfiles?.filter(profile => profile.browserType === currentSelectedBrowser)
  const [selectedProfiles, setSelectedProfiles] = React.useState<Set<number>>(new Set())

  const handleBackButton = () => {
    setViewType(ViewType.ImportSelectBrowser)
  }

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { id, checked } = e.target
    const parsedId = parseInt(id.split('-')[1])

    if (!checked && selectedProfiles?.has(parsedId)) {
      selectedProfiles.delete(parsedId)
      setSelectedProfiles(new Set([...selectedProfiles]))
      return
    }

    setSelectedProfiles(new Set(selectedProfiles.add(parsedId)))
  }

  const selectAll = () => {
    setSelectedProfiles(new Set(filteredProfiles?.map(profile => profile.index)))
  }

  const handleImportProfiles = () => {
    if (selectedProfiles.size <= 0) return

    selectedProfiles.forEach((entry) => {
      ImportDataBrowserProxyImpl.getInstance().importData(entry, defaultImportTypes)
      incrementCount()
    })

    WelcomeBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.ImportSelectProfile, isFinished: false, isSkipped: false })
  }

  return (
    <S.MainBox>
      <div className="view-header-box">
        <div className="view-header-actions">
          <button className="back-button" onClick={handleBackButton}>
            <LeftArrowSVG />
            <span>{getLocale('braveWelcomeBackButtonLabel')}</span>
          </button>
        </div>
        <div className="view-details">
          <h1 className="view-title">{getLocale('braveWelcomeSelectProfileLabel')}</h1>
          <p className="view-desc">{getLocale('braveWelcomeSelectProfileDesc')}</p>
        </div>
      </div>
      <S.ProfileListBox>
        <div className="profile-list">
          <div className="list-actions">
            <div className="right">
              <button onClick={selectAll}>{getLocale('braveWelcomeSelectAllButtonLabel')}</button>
            </div>
          </div>
          {filteredProfiles?.map(entry => {
            return (<ProfileItem
              key={entry.index}
              id={entry.index}
              profileName={entry.name}
              onChange={handleChange}
              isChecked={selectedProfiles.has(entry.index)}
            />)
          })}
        </div>
      </S.ProfileListBox>
      <S.ActionBox>
        <Button
          isPrimary={true}
          onClick={handleImportProfiles}
          scale="jumbo"
        >
          {getLocale('braveWelcomeImportProfilesButtonLabel')}
        </Button>
      </S.ActionBox>
    </S.MainBox>
  )
}

export default SelectProfile
