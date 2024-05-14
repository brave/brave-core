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
import { useViewTypeTransition } from '../../state/hooks'
import { WelcomeBrowserProxyImpl, ImportDataBrowserProxyImpl, defaultImportTypes, P3APhase } from '../../api/welcome_browser_proxy'
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
  const {
    viewType,
    setViewType,
    incrementCount,
    currentSelectedBrowserProfiles
  } = React.useContext(DataContext)
  const [selectedProfiles, setSelectedProfiles] = React.useState<Set<number>>(new Set())

  const { back } = useViewTypeTransition(viewType)
  const handleBackButton = () => setViewType(back!)

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
    setSelectedProfiles(
      new Set(currentSelectedBrowserProfiles?.map((profile) => profile.index))
    )
  }

  const handleImportProfiles = () => {
    if (selectedProfiles.size <= 0) return
    let entries: number[] = []
    selectedProfiles.forEach((entry) => {
      entries.push(entry)
      incrementCount()
    })

    if (entries.length === 1) {
      ImportDataBrowserProxyImpl.getInstance().importData(entries[0], defaultImportTypes)
    } else {
      ImportDataBrowserProxyImpl.getInstance().importDataBulk(entries, defaultImportTypes)
    }
    WelcomeBrowserProxyImpl.getInstance().recordP3A(P3APhase.Consent)
  }
  const getImportEntryName = (entry: any) => {
    let name = entry.name
    if (entry.profileName) {
      name += ' - ' + entry.profileName
    }
    return name
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
          {currentSelectedBrowserProfiles?.map((entry) => {
            return (<ProfileItem
              key={entry.index}
              id={entry.index}
              profileName={getImportEntryName(entry)}
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
