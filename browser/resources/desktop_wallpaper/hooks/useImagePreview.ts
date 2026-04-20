// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect, useRef, useState } from 'react'
import { DisplayInfos } from 'gen/brave/components/desktop_wallpaper/desktop_wallpaper.mojom.m.js'

export function useImagePreview(
  imageUrl: string | null,
  monitor: string,
  displays: Array<DisplayInfos>,
  fit: string,
) {
  const containerRef = useRef<HTMLDivElement>(null)
  const [imgSize, setImgSize] = useState({ width: 0, height: 0 })
  const [scale, setScale] = useState(1)
  const selectedMonitor = displays.find((d) => d.id === monitor)

  const monitorWidth = selectedMonitor?.width ?? 1920
  const monitorHeight = selectedMonitor?.height ?? 1080

  const updateScale = () => {
    if (containerRef.current) {
      const rect = containerRef.current.getBoundingClientRect()
      setScale(rect.width / Number(monitorWidth))
    }
  }

  const getBackgroundSize = () => {
    switch (fit) {
      case 'fill':
        return 'cover'
      case 'fit':
        return 'contain'
      case 'stretch':
        return '100% 100%'
      case 'center':
        return `${imgSize.width * scale}px ${imgSize.height * scale}px`
      default:
        return 'cover'
    }
  }

  // Load the image into the preview box
  useEffect(() => {
    if (imageUrl) {
      const img = new Image()
      img.onload = () =>
        setImgSize({ width: img.naturalWidth, height: img.naturalHeight })
      img.src = imageUrl
    }
  }, [imageUrl])

  // Apply scale
  useEffect(() => {
    updateScale()
    window.addEventListener('resize', updateScale)
    return () => window.removeEventListener('resize', updateScale)
  }, [updateScale])

  return {
    monitorWidth,
    monitorHeight,
    containerRef,
    getBackgroundSize,
  }
}
