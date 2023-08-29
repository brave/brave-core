// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled, { css } from 'styled-components'
import { Link } from 'react-router-dom'

import Icon from '@brave/leo/react/icon'
import { color, font, radius, spacing } from '@brave/leo/tokens/css'
import LeoButton from '@brave/leo/react/button'

import PlaylistInfo from './playlistInfo'
import {
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

const StyledLink = styled(Link)`
  text-decoration: none;
  color: unset;
`

interface HeaderProps {
  playlistId?: string
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
  margin-right: calc(-1 * (${spacing.xl} - ${spacing.m}))
`

const ColoredIcon = styled(Icon)<{ color: string }>`
  color: ${p => p.color};
  ${iconSize}
`

const ProductNameContainer = styled.div`
  flex-grow: 1;
  display: flex;
  gap: 4px;
  font: ${font.primary.heading.h4};
`

const ColoredSpan = styled.span<{ color: string }>`
  color: ${p => p.color};
`

const HeaderContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  border-bottom: 1px solid ${color.divider.subtle};
  background-color: ${color.container.background};
  height: 100%;
  padding: 0 ${spacing.xl};
  gap: ${spacing.xl};
`

const StyledPlaylistInfo = styled(PlaylistInfo)`
  flex-grow: 1;
`

const LeoButtonContainer = styled.button`
  display: contents;
  cursor: pointer;
`

const StyledInput = styled.input`
  flex-grow: 1;
  color: ${color.text.primary};
  font: ${font.primary.heading.h4};
  border: none;
  border-radius: ${radius[8]};
  background: ${color.container.highlight};
  padding: 10px 8px;
`

const SaveButton = styled(LeoButton)`
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
    <LeoButtonContainer
      onClick={() => getPlaylistActions().setPlaylistEditMode(undefined)}
    >
      <ColoredIcon name='arrow-left' color={color.icon.default} />
    </LeoButtonContainer>
  ) : (
    <StyledLink to='/'>
      <ColoredIcon name='arrow-left' color={color.icon.default} />
    </StyledLink>
  )
}

function PlaylistHeader ({ playlistId }: { playlistId: string }) {
  const playlist = usePlaylist(playlistId)
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

    const uncachedItems = playlist.items.filter(item => !item.cached)
    if (uncachedItems.length) {
      contextualMenuItems.push({
        name: getLocalizedString(
          'bravePlaylistContextMenuKeepForOfflinePlaying'
        ),
        iconName: 'cloud-download',
        onClick: () => {
          uncachedItems.forEach(item =>
            getPlaylistAPI().recoverLocalData(item.id)
          )
        }
      })
    }

    const playedItems = playlist.items.filter(
      item => item.lastPlayedPosition >= Math.floor(+item.duration / 1e6)
    )
    if (playedItems.length) {
      contextualMenuItems.push({
        name: getLocalizedString(
          'bravePlaylistContextMenuRemovePlayedContents'
        ),
        iconName: 'list-checks',
        onClick: () => {
          playedItems.forEach(item =>
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
          <StyledInput
            type='text'
            defaultValue={playlist.name}
            autoFocus
            onChange={e => setNewName(e.target.value)}
            onKeyDown={e => {
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
    <LeoButtonContainer
      onClick={() => {
        getPlaylistAPI().showCreatePlaylistUI()
      }}
    >
      <ColoredIcon name='plus-add' color={color.icon.default} />
    </LeoButtonContainer>
  )
}

function SettingButton () {
  return (
    <LeoButtonContainer onClick={() => {}}>
      <ColoredIcon name='settings' color={color.icon.default} />
    </LeoButtonContainer>
  )
}

function PlaylistsCatalogHeader () {
  return (
    <>
      <GradientIcon name='product-playlist-bold-add-color' />
      <ProductNameContainer>
        <ColoredSpan color={color.text.secondary}>Brave</ColoredSpan>
        <ColoredSpan color={color.text.primary}>Playlist</ColoredSpan>
      </ProductNameContainer>
      <NewPlaylistButton />
      <SettingButton />
    </>
  )
}

export default function Header ({ playlistId }: HeaderProps) {
  return (
    <HeaderContainer>
      {playlistId ? (
        <PlaylistHeader playlistId={playlistId} />
      ) : (
        <PlaylistsCatalogHeader />
      )}
    </HeaderContainer>
  )
}
