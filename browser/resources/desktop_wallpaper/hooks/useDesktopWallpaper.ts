// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect } from 'react'

import { callbackRouter, handler } from '../desktop_wallpaper_startup'
import { SelectItemEventDetail } from '@brave/leo/types/src/components/menu/menu.svelte'
import {
  Scaling,
  WallpaperStatus,
  DisplayInfos,
} from 'gen/brave/components/desktop_wallpaper/desktop_wallpaper.mojom.m.js'

import { useDesktopWallpaperState } from './useDesktopWallpaperState'

export function useDesktopWallpaper(defaultMonitorKey: string) {
  const [state, dispatch] = useDesktopWallpaperState(defaultMonitorKey)
  const {
    status,
    fit,
    monitor,
    useAllMonitors,
    imageSource,
    displays,
    path,
    imageUrl,
  } = state

  const fitToScaling: Record<string, Scaling> = {
    fill: Scaling.kFillScreen,
    fit: Scaling.kFitToScreen,
    stretch: Scaling.kStretchToFill,
    center: Scaling.kCenter,
  }

  const handleApply = () => {
    if (!path) {
      dispatch({
        type: 'SET_STATUS',
        payload: { type: 'error', message: 'Cannot find path' },
      })
      return
    }

    const targetDisplays = useAllMonitors
      ? displays
      : displays.filter((d) => d.id === monitor)

    handler.setImageAsDesktopWallpaper(
      path,
      targetDisplays,
      fitToScaling[fit] ?? Scaling.kFillScreen,
    )
  }

  const handleCancel = () => {
    chrome.send('dialogClose')
  }

  const handleFitOnChange = (evt: SelectItemEventDetail) => {
    if (evt.value) {
      dispatch({ type: 'SET_FIT', payload: evt.value })
    }
  }

  const handleUseBackgroundOnChange = ({ checked }: { checked: boolean }) => {
    dispatch({ type: 'SET_USE_ALL_MONITORS', payload: checked })
  }

  const handleMonitorOnChange = (evt: SelectItemEventDetail) => {
    if (evt.value) {
      dispatch({ type: 'SET_MONITOR', payload: evt.value })
    }
  }

  useEffect(() => {
    if (!imageUrl) {
      return
    }

    handler.fetchImage(imageUrl)
    handler.getDisplayInfos()

    const receiveImageListener = (
      img: string,
      ext: string,
      imagePath: string,
    ) => {
      dispatch({
        type: 'RECEIVE_IMAGE',
        payload: {
          dataUrl: `data:image/${ext};base64,${img}`,
          path: imagePath,
        },
      })
    }

    const receiveCloseListener = (wallpaperStatus: WallpaperStatus) => {
      if (wallpaperStatus !== WallpaperStatus.success) {
        dispatch({
          type: 'SET_STATUS',
          payload: {
            type: 'error',
            message: 'An error occurred.',
          },
        })
        return
      }

      handleCancel()
    }

    const imageListenerId =
      callbackRouter.receiveImage.addListener(receiveImageListener)
    const displayListenerId = callbackRouter.receiveDisplayInfos.addListener(
      (d: Array<DisplayInfos>) =>
        dispatch({ type: 'SET_DISPLAYS', payload: d }),
    )
    const closeListenerId =
      callbackRouter.receiveWallpaperStatus.addListener(receiveCloseListener)

    return () => {
      callbackRouter.receiveImage.removeListener(imageListenerId)
      callbackRouter.receiveDisplayInfos.removeListener(displayListenerId)
      callbackRouter.receiveWallpaperStatus.removeListener(closeListenerId)
    }
  }, [imageUrl])

  return {
    status,
    fit,
    monitor,
    useAllMonitors,
    imageSource,
    displays,
    handleApply,
    handleCancel,
    handleFitOnChange,
    handleUseBackgroundOnChange,
    handleMonitorOnChange,
  }
}
