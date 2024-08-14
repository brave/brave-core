// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled, { css } from 'styled-components'
import { Link } from 'react-router-dom'

import Icon from '@brave/leo/react/icon'
import {
  color,
  font,
  radius,
  spacing,
  icon,
  elevation
} from '@brave/leo/tokens/css/variables'
import LeoButton from '@brave/leo/react/button'

import PlaylistInfo from './playlistInfo'
import {
  ApplicationState,
  CachingProgress,
  PlaylistEditMode,
  usePlaylist,
  usePlaylistEditMode,
  useTotalDuration,
  useTotalSize
} from '../reducers/states'
import ContextualMenuAnchorButton from './contextualMenu'
import { getPlaylistAPI } from '../api/api'
import { getLocalizedString } from '../utils/l10n'
import { getPlaylistActions } from '../api/getPlaylistActions'
import { useSelector } from 'react-redux'

const StyledLink = styled(Link)`
  text-decoration: none;
  color: unset;
`

interface HeaderProps {
  playlistId?: string
  className?: string
}

const iconSize = css`
  --leo-icon-size: 20px;
`

const GradientIcon = styled(Icon)`
  --leo-icon-color: linear-gradient(
    314.42deg,
    #fa7250 8.49%,
    #ff1893 43.72%,
    #a78aff 99.51%
  );
  ${iconSize}
  margin-right: calc(-1 * (${spacing.l} - ${spacing.m}));
`

const ColoredIcon = styled(Icon)<{ color: string }>`
  color: ${(p) => p.color};
  ${iconSize}
`

const ProductNameContainer = styled.div`
  flex-grow: 1;
  font: ${font.heading.h4};
`

const ColoredSpan = styled.span<{ color: string }>`
  color: ${(p) => p.color};
`

const HeaderContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  border-bottom: 1px solid ${color.divider.subtle};
  background-color: ${color.container.background};
  height: 100%;
  width: 100vw;
  box-sizing: border-box;
  gap: ${spacing.l};

  & > :first-child {
    margin-left: ${spacing.xl};
  }
  & > :last-child {
    margin-right: ${spacing.xl};
  }
`

const StyledPlaylistInfo = styled(PlaylistInfo)`
  flex-grow: 1;
`

const StyledButton = styled(LeoButton)`
  flex: 0 0 auto;
`

const MediumSizedButton = styled(StyledButton)`
  width: var(--leo-icon-m);
  height: var(--leo-icon-m);
`

const StyledSeparator = styled.div`
  width: ${elevation.xxs};
  background-color: ${color.divider.subtle};
  height: ${icon.m};
`

const NameEditFieldContainer = styled.div`
  position: relative;
  flex-grow: 1;
  min-width: 0px;
`

const StyledInput = styled.input`
  color: ${color.text.primary};
  font: ${font.heading.h4};
  border: none;
  border-radius: ${radius[8]};
  background: ${color.container.highlight};
  padding: 8px 50px 8px 10px;
  width: 100%;
  box-sizing: border-box;
`

const StyledLengthLabel = styled.span`
  color: ${color.text.secondary};
  position: absolute;
  right: 8px;
  top: 50%;
  transform: translateY(-50%);
  cursor: text;
  user-select: none;
`

const SaveButton = styled(StyledButton)`
  width: fit-content;
  min-width: 72px;
  flex-grow: 0;
  --leo-button-padding: 10px;
`

function BackButton ({
  playlistEditMode
}: {
  playlistEditMode?: PlaylistEditMode
}) {
  return playlistEditMode === PlaylistEditMode.BULK_EDIT ? (
    <StyledButton
      size='small'
      kind='plain'
      fab
      onClick={() => getPlaylistActions().setPlaylistEditMode(undefined)}
    >
      <ColoredIcon
        name='arrow-left'
        color={color.icon.default}
      />
    </StyledButton>
  ) : (
    <StyledLink to='/'>
      <ColoredIcon
        name='arrow-left'
        color={color.icon.default}
      />
    </StyledLink>
  )
}

const maxNameLength = 30

function NameEditField ({
  defaultName,
  onSave,
  onChange
}: {
  defaultName: string
  onSave: () => void
  onChange: (newName: string) => void
}) {
  const inputRef = React.useRef<HTMLInputElement>(null)
  const [currentLength, setCurrentLength] = React.useState(defaultName.length);
  
  return (
    <NameEditFieldContainer>
      <StyledInput
        ref={inputRef}
        type='text'
        maxLength={maxNameLength}
        defaultValue={defaultName}
        autoFocus
        onChange={(e) => { 
          onChange(e.target.value) 
          setCurrentLength(e.target.value.length)
        }}
        onKeyDown={(e) => {
          if (e.key === 'Escape') {
            getPlaylistActions().setPlaylistEditMode(undefined)
            e.preventDefault()
            return
          }

          if (e.key === 'Enter') {
            onSave()
            e.preventDefault()
          }
        }}
      />
      <StyledLengthLabel onClick={() => {inputRef.current?.focus()}}>
        {currentLength}/{maxNameLength}
      </StyledLengthLabel>
    </NameEditFieldContainer>
  )
}

function PlaylistHeader ({ playlistId }: { playlistId: string }) {
  const playlist = usePlaylist(playlistId)
  const cachingProgress = useSelector<
    ApplicationState,
    Map<string, CachingProgress> | undefined
  >((applicationState) => applicationState.playlistData?.cachingProgress)

  const contextualMenuItems = []
  if (playlist?.items.length) {
    contextualMenuItems.push({
      name: getLocalizedString('bravePlaylistContextMenuEdit'),
      iconName: 'list-bullet-default',
      onClick: () =>
        getPlaylistActions().setPlaylistEditMode(PlaylistEditMode.BULK_EDIT)
    })

    // TODO(sko) We don't support this yet.
    // contextualMenuItems.push({ name: 'Share', iconName: 'share-macos', onClick: () => {} })

    const uncachedItems = playlist.items.filter(
      (item) => !item.cached && !cachingProgress?.has(item.id)
    )

    if (uncachedItems.length) {
      contextualMenuItems.push({
        name: getLocalizedString(
          'bravePlaylistContextMenuKeepForOfflinePlaying'
        ),
        iconName: 'cloud-download',
        onClick: () => {
          uncachedItems.forEach((item) =>
            getPlaylistAPI().recoverLocalData(item.id)
          )
        }
      })
    }

    const playedItems = playlist.items.filter(
      (item) => item.lastPlayedPosition >= Math.floor(+item.duration / 1e6)
    )
    if (playedItems.length) {
      contextualMenuItems.push({
        name: getLocalizedString(
          'bravePlaylistContextMenuRemovePlayedContents'
        ),
        iconName: 'list-checks',
        onClick: () => {
          playedItems.forEach((item) =>
            getPlaylistAPI().removeItemFromPlaylist(playlistId, item.id)
          )
        }
      })
    }
  }

  const isDefaultPlaylist = playlist?.id === 'default'
  if (contextualMenuItems && !isDefaultPlaylist) {
    contextualMenuItems.unshift({
      name: getLocalizedString('bravePlaylistContextMenuRenamePlaylist'),
      iconName: 'edit-box',
      onClick: () => {
        getPlaylistActions().setPlaylistEditMode(PlaylistEditMode.RENAME)
      }
    })
    contextualMenuItems.push({
      name: getLocalizedString('bravePlaylistContextMenuDeletePlaylist'),
      iconName: 'trash',
      onClick: () => {
        getPlaylistAPI().showRemovePlaylistUI(playlistId)
      }
    })
  }

  const totalDuration = useTotalDuration(playlist)
  const totalSize = useTotalSize(playlist)
  const playlistEditMode = usePlaylistEditMode()

  const [newName, setNewName] = React.useState(playlist?.name)
  const hasNewName = !!newName && newName !== playlist?.name

  const onSave = () => {
    if (!playlist || !hasNewName) {
      return
    }

    getPlaylistAPI().renamePlaylist(playlist.id!, newName)
    getPlaylistActions().setPlaylistEditMode(undefined)
  }

  React.useEffect(() => {
    // on unmount, resets 'edit mode'.
    return () => {
      getPlaylistActions().setPlaylistEditMode(undefined)
    }
  }, [])

  return !playlist ? null : (
    <>
      <BackButton playlistEditMode={playlistEditMode} />
      {playlistEditMode === PlaylistEditMode.RENAME ? (
        <>
          <NameEditField
            defaultName={playlist?.name}
            onChange={setNewName}
            onSave={onSave}
          />
          <SaveButton
            kind='filled'
            size='small'
            onClick={onSave}
            isDisabled={!hasNewName}
          >
            Save
          </SaveButton>
        </>
      ) : (
        <>
          <StyledPlaylistInfo
            isDefaultPlaylist={isDefaultPlaylist}
            itemCount={playlist.items.length}
            playlistName={playlist.name}
            totalDuration={totalDuration}
            totalSize={totalSize}
            nameColor={color.text.primary}
            detailColor={color.text.secondary}
          />
          <ContextualMenuAnchorButton
            visible={!!contextualMenuItems.length}
            items={contextualMenuItems}
          />
        </>
      )}
    </>
  )
}

function NewPlaylistButton () {
  return (
    <MediumSizedButton
      size='small'
      kind='plain'
      fab
      title={getLocalizedString('bravePlaylistTooltipCreatePlaylistFolder')}
      onClick={() => {
        getPlaylistAPI().showCreatePlaylistUI()
      }}
    >
      <ColoredIcon
        name='folder-new'
        color={color.icon.default}
      />
    </MediumSizedButton>
  )
}

function SettingButton () {
  return (
    <MediumSizedButton
      size='small'
      kind='plain'
      fab
      title={getLocalizedString('bravePlaylistTooltipOpenPlaylistSettings')}
      onClick={() => getPlaylistAPI().openSettingsPage()}
    >
      <ColoredIcon
        name='settings'
        color={color.icon.default}
      />
    </MediumSizedButton>
  )
}

function CloseButton () {
  return (
    <MediumSizedButton
      size='small'
      kind='plain'
      fab
      title={getLocalizedString('bravePlaylistTooltipClosePanel')}
      onClick={() => getPlaylistAPI().closePanel()}
    >
      <ColoredIcon
        name='close'
        color={color.icon.default}
      />
    </MediumSizedButton>
  )
}

function PlaylistsCatalogHeader () {
  return (
    <>
      <GradientIcon name='product-playlist-bold-add-color' />
      <ProductNameContainer>
        <ColoredSpan color={color.text.primary}>Playlist</ColoredSpan>
      </ProductNameContainer>
      <NewPlaylistButton />
      <SettingButton />
      <StyledSeparator />
      <CloseButton />
    </>
  )
}

export default function Header ({ playlistId, className }: HeaderProps) {
  return (
    <HeaderContainer className={className}>
      {playlistId ? (
        <PlaylistHeader playlistId={playlistId} />
      ) : (
        <PlaylistsCatalogHeader />
      )}
    </HeaderContainer>
  )
}
