// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import { Link } from 'react-router-dom';

import Icon from '@brave/leo/react/icon'
import { color } from '@brave/leo/tokens/css'

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import PlayLaterCardOverlayImage from '../assets/playlater-card-overlay.svg'
import PlaylistInfo from './playlistInfo'

interface CatalogProps {
  playlists: PlaylistMojo.Playlist[]
}

interface ThumbnailProps {
  isDefaultPlaylist: boolean
  thumbnailUrl: string | undefined
}

const StyledLink = styled(Link)<{}>`
  text-decoration: none;
  color:unset
`

const PlaylistContainer = styled.div<ThumbnailProps>`
  /* Default Playlist background */
  ${ p => p.isDefaultPlaylist && css`background: linear-gradient(150.64deg, #322FB4 0%, #3835CA 100%);`}

  /* Default background for playlist without thumbnail */
  ${ p => !p.isDefaultPlaylist && !p.thumbnailUrl && css`background-color: ${color.light.container.highlight};`}

  /* Playlist with Thumbnail*/
  ${ p => !p.isDefaultPlaylist && p.thumbnailUrl && css`
      background: linear-gradient(0deg, rgba(0, 0, 0, 0.4), rgba(0, 0, 0, 0.4)), url(${ p.thumbnailUrl });
      background-size: cover;` }
  border-radius: 8px;
  padding: 8px;

  position:relative;
  display:flex;
  gap: 16px;
`

const PlayLaterCardOverlay = styled.div<{}>`
  position: absolute;
  top: 0;
  right: 0;
  content: url(${PlayLaterCardOverlayImage});
`

const ColoredPlaylistInfo = styled(PlaylistInfo)<{ hasBackground: boolean }>`
  color: ${ ({hasBackground}) => hasBackground ? color.dark.text.primary : color.light.text.primary };
`

const PlaylistThumbnailContainer = styled.div<ThumbnailProps>`
  /* Default Playlist icon */
  ${ p => p.isDefaultPlaylist && css`background-color: ${color.light.interaction.buttonPrimaryBackground}; color: white;` }

  /* Default icon for playlist without thumbnail */
  ${ p => !p.isDefaultPlaylist && !p.thumbnailUrl && css`background-color: ${color.light.container.background}; `}

  /* Playlist with Thumbnail*/
  ${ p => !p.isDefaultPlaylist && !!p.thumbnailUrl && css`
    background-image: url(${p.thumbnailUrl}); 
    background-size: contain;
    background-repeat: no-repeat;
    background-position: center;` }

  width: 80px;
  height: 80px;

  display: flex;
  justify-content: center;
  align-items: center;
`
function PlaylistThumbnail (props : ThumbnailProps) {
    return (
      <PlaylistThumbnailContainer {...props}>
        {
          (props.isDefaultPlaylist || !props.thumbnailUrl) && 
          <Icon name={props.isDefaultPlaylist ? "history" : "product-playlist"} />
        }
      </PlaylistThumbnailContainer>
    )
}

// A card UI for representing a playlist.
function PlaylistCard ({playlist} : {playlist: PlaylistMojo.Playlist}) {
  const isDefaultPlaylist = React.useMemo(() => {
    return playlist.id === "default"
  }, [playlist])

  const thumbnailUrl = React.useMemo(() => {
    if (!playlist.items) return undefined 

    return playlist.items?.find(item => item.thumbnailPath?.url)?.thumbnailPath.url
  }, [playlist, playlist.items])

  const hasBackground = React.useMemo(() => {
    return isDefaultPlaylist || !!thumbnailUrl
  }, [isDefaultPlaylist, thumbnailUrl]);

  const totalDuration = React.useMemo(() => {
    // TODO(sko) Duration value is not correct now.
    //  * We need to update duration when Playlist player plays a video
    //  * Check if duration is converted well. the duration value is formatted
    //    as string based on base::Time.
    if (!playlist.items?.length) return 0

    return playlist.items.reduce((sum, item) => {
      return sum + parseInt(item.duration)
    }, 0)
  }, [playlist, playlist.items])

  return (
    <StyledLink to={`/playlist/${playlist.id}`}>
      <PlaylistContainer isDefaultPlaylist={ isDefaultPlaylist } thumbnailUrl={thumbnailUrl} >
        { isDefaultPlaylist && <PlayLaterCardOverlay /> }
        <PlaylistThumbnail isDefaultPlaylist={ isDefaultPlaylist } thumbnailUrl={thumbnailUrl} />
        <ColoredPlaylistInfo playlistName={playlist.name}
                             isDefaultPlaylist={isDefaultPlaylist}
                             itemCount={playlist.items.length}
                             totalDuration={totalDuration}
                             hasBackground={hasBackground} />
      </PlaylistContainer>
    </StyledLink>
  )
}

const PlaylistsCatalogFlexBox = styled.div<{}>`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

// A catalog that shows all playlists
export default function PlaylistsCatalog ({playlists}: CatalogProps) {
    return (
      <PlaylistsCatalogFlexBox>
        { playlists.map((playlist: PlaylistMojo.Playlist) => { return <PlaylistCard key={playlist.id} playlist={ playlist }/> }) }
      </PlaylistsCatalogFlexBox>)
}
