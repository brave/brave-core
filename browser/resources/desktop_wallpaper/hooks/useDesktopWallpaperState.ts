import { useReducer } from 'react'

import { DisplayInfos } from 'gen/brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.m.js'

export type DesktopWallpaperStatus = {
  type: 'loading' | 'error' | 'success' | 'idle'
  message?: string
}

export type DesktopWallpaperState = {
  status: DesktopWallpaperStatus
  fit: string
  monitor: string
  useAllMonitors: boolean
  imageSource: string | null
  displays: DisplayInfos[]
  path: string | null
  imageUrl: string | undefined
}

export type DesktopWallpaperAction =
  | { type: 'SET_STATUS'; payload: DesktopWallpaperStatus }
  | { type: 'SET_FIT'; payload: string }
  | { type: 'SET_MONITOR'; payload: string }
  | { type: 'SET_USE_ALL_MONITORS'; payload: boolean }
  | { type: 'SET_DISPLAYS'; payload: Array<DisplayInfos> }
  | {
    type: 'RECEIVE_IMAGE'
    payload: { dataUrl: string; path: string }
  }

export function desktopWallpaperReducer(
  state: DesktopWallpaperState,
  action: DesktopWallpaperAction
): DesktopWallpaperState {
  switch (action.type) {
    case 'SET_STATUS':
      return { ...state, status: action.payload }
    case 'SET_FIT':
      return { ...state, fit: action.payload }
    case 'SET_MONITOR':
      return { ...state, monitor: action.payload }
    case 'SET_USE_ALL_MONITORS':
      return { ...state, useAllMonitors: action.payload }
    case 'SET_DISPLAYS':
      return { ...state, displays: action.payload }
    case 'RECEIVE_IMAGE':
      return {
        ...state,
        imageSource: action.payload.dataUrl,
        path: action.payload.path,
        status: { type: 'idle' },
      }
    default:
      return state
  }
}

function getInitialState(defaultMonitorKey: string): DesktopWallpaperState {
  const initialStatus: DesktopWallpaperState = {
    status: { type: 'loading' },
    fit: 'fill',
    monitor: defaultMonitorKey,
    useAllMonitors: true,
    imageSource: null,
    displays: [],
    path: null,
    imageUrl: undefined,
  }

  const args = JSON.parse(chrome.getVariableValue('dialogArguments'))

  return {
    ...initialStatus,
    imageUrl: args.image_url,
  }
}

export function useDesktopWallpaperState(defaultMonitorKey: string) {
  return useReducer(desktopWallpaperReducer, defaultMonitorKey, getInitialState)
}
