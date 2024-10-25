/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { msToMinutesAndSeconds } from './utils'

/**
 * In this case, "Attachments" refers to any of the media
 * entities that are associated with the post.
 * TODO (Sampson): Consider combining "cards" into this
 * category as well.
 */
export function distillPostMediaEntities(post: any) {
  const media = post.extended_entities.media
    .map((media: any) => {
      switch (media.type) {
        case 'photo':
        case 'video':
        case 'animated_gif':
          return distillPostMediaEntity(media)
        default:
          console.warn(`Unhandled media type: ${media.type}`, media)
          return null
      }
    })
    .filter(Boolean)
    .join('\n')

  return media
}

function distillPostMediaEntity(media: any) {
  const { type, expanded_url: expandedURL, video_info: videoInfo } = media

  const { width, height } = media.original_info ?? {}
  const { duration_millis: durationMillis, variants } = videoInfo ?? {}

  const labels = {
    photo: 'Photo',
    video: 'Video',
    animated_gif: 'Animated GIF'
  } as Record<string, string>

  const downloadableVariant = variants && getDownloadableVariant(variants)
  const output = [` - Type: ${labels[type] ?? type}`]

  if (width && height) output.push(` - Dimensions: ${width}x${height}`)

  if (durationMillis)
    output.push(` - Duration: ${msToMinutesAndSeconds(durationMillis)}s`)

  if (expandedURL) output.push(` - URL: ${expandedURL}`)

  if (downloadableVariant)
    output.push(` - Download URL: ${downloadableVariant.url}`)

  return output.filter(Boolean).join('\n')
}

function getDownloadableVariant(variants: any) {
  return variants.reduce((best: any, variant: any) => {
    if (variant.content_type === 'video/mp4') {
      return best === null || variant.bitrate > best.bitrate ? variant : best
    }
    return best
  }, null)
}

/**
 * We will use this to replace shortened URLs
 * in both tweet bodies and profile descriptions.
 */
export function expandEntities(text: string, entities: any) {
  let output = text

  for (const name in entities) {
    if (['media', 'urls'].includes(name)) {
      for (const { url, expanded_url: xURL } of entities[name]) {
        /**
         * Media attachments are already promoted to a top-level
         * listing, so we will simply remove them as inline
         * mentions. URLs, on the other hand will be replaced
         * with their fully-expanded value.
         */
        output = output.replace(url, name === 'media' ? '' : xURL)
      }
    }
  }

  return output.trim()
}
