// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getBraveNewsController from '../../../../brave_news/browser/resources/shared/api'
import { useVisible, VisibleOptions } from '$web-common/useVisible'

interface Options {
  rootElement?: HTMLElement | null
  rootMargin?: string
  threshold?: number

  useCache?: boolean
  onLoaded?: () => any
}

const cache: { [url: string]: string } = {}
function useFavicon (publisherId: string | undefined) {
  const [url, setUrl] = React.useState<string>(cache[publisherId!])
  React.useEffect(() => {
    if (!publisherId) {
      return
    }

    if (cache[publisherId]) {
      setUrl(cache[publisherId])
      return
    }

    let cancelled = false
    getBraveNewsController().getFavIconData(publisherId).then(({ imageData }) => {
      if (cancelled) return
      if (!imageData) return

      const blob = new Blob([new Uint8Array(imageData)], { type: 'image/*' })
      const url = URL.createObjectURL(blob)
      cache[publisherId] = url
      setUrl(url)
    })

    return () => {
      cancelled = true
    }
  }, [publisherId])

  return url
}

export function useLazyFavicon (publisherId: string, options: VisibleOptions) {
  const { visible, setElementRef } = useVisible(options)
  const url = useFavicon(visible ? publisherId : undefined)
  return {
    url,
    setElementRef
  }
}

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
  const { visible, setElementRef } = useVisible(options)

  return {
    url: useUnpaddedImageUrl(visible ? paddedUrl : undefined, options.onLoaded, options.useCache) || cache[paddedUrl!],
    setElementRef
  }
}
