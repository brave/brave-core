// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getBraveNewsController from '../../../api/brave_news'

interface Options {
  rootElement?: HTMLElement | null
  rootMargin?: string
  threshold?: number

  useCache?: boolean
  onLoaded?: () => any
}

const cache: { [url: string]: string } = {}
export function useUnpaddedImageUrl (paddedUrl: string | undefined, onLoaded?: () => any, useCache?: boolean) {
  const [unpaddedUrl, setUnpaddedUrl] = React.useState('')

  React.useEffect(() => {
    const onReceiveUnpaddedUrl = (result: string) => {
      if (useCache) cache[paddedUrl!] = result
      setUnpaddedUrl(result)

      if (onLoaded) window.requestAnimationFrame(() => onLoaded())
    }

    // Storybook method
    // @ts-expect-error
    if (window.braveStorybookUnpadUrl) {
      // @ts-expect-error
      window.braveStorybookUnpadUrl(paddedUrl)
        .then(onReceiveUnpaddedUrl)
      return
    }

    if (!paddedUrl) return

    if (cache[paddedUrl]) {
      onReceiveUnpaddedUrl(cache[paddedUrl])
      return
    }

    let blobUrl: string
    getBraveNewsController().getImageData({ url: paddedUrl })
      .then(async (result) => {
        if (!result.imageData) {
          return
        }

        const blob = new Blob([new Uint8Array(result.imageData)], { type: 'image/*' })
        blobUrl = URL.createObjectURL(blob)
        onReceiveUnpaddedUrl(blobUrl)
      })
      .catch(err => {
        console.error(`Error getting image for ${paddedUrl}.`, err)
      })

      // Only revoke the URL if we aren't using the cache.
      return () => {
        if (!useCache) URL.revokeObjectURL(blobUrl)
      }
  }, [paddedUrl])
  return unpaddedUrl
}

export function useLazyUnpaddedImageUrl (paddedUrl: string | undefined, options: Options) {
  const [visible, setVisible] = React.useState(false)
  const [elementRef, setElementRef] = React.useState<HTMLElement | null>(null)

  React.useEffect(() => {
    if (!elementRef) return

    const observer = new IntersectionObserver(([intersectionInfo]) => {
      if (!intersectionInfo.isIntersecting) {
        return
      }
      setVisible(true)
    }, {
      root: options.rootElement,
      rootMargin: options.rootMargin,
      threshold: options.threshold
    })

  observer.observe(elementRef)
    return () => {
      observer.disconnect()
    }
  }, [options.rootElement, elementRef])

  return {
    url: useUnpaddedImageUrl(visible ? paddedUrl : undefined, options.onLoaded, options.useCache) || cache[paddedUrl!],
    elementRef: setElementRef
  }
}
