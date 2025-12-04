/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import type { ImageResult } from './deep_research_types'
import styles from './enrichments.module.scss'

interface ImageResultsProps {
  images: ImageResult[]
}

const MAX_IMAGES_DISPLAYED = 8

const ImageResultsEvent = React.memo(function ImageResultsEvent({ images }: ImageResultsProps) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const visibleImages = isExpanded ? images : images.slice(0, MAX_IMAGES_DISPLAYED)
  const hiddenCount = Math.max(images.length - MAX_IMAGES_DISPLAYED, 0)

  const handleImageClick = (url: string) => {
    window.open(url, '_blank', 'noopener,noreferrer')
  }

  return (
    <div className={styles.enrichmentSection}>
      <div className={styles.enrichmentHeader}>
        <Icon name='image' />
        <span>Images</span>
      </div>
      <div className={styles.imagesGrid}>
        {visibleImages.map((image, idx) => (
          <button
            key={idx}
            className={styles.imageCard}
            onClick={() => handleImageClick(image.url)}
            title={image.title}
          >
            <img
              src={image.thumbnail_url}
              alt={image.title}
              loading='lazy'
              className={styles.imageThumbnail}
            />
          </button>
        ))}
      </div>
      {hiddenCount > 0 && !isExpanded && (
        <button
          className={styles.expandButton}
          onClick={() => setIsExpanded(true)}
        >
          <Icon name='plus-add' />
          Show {hiddenCount} more images
        </button>
      )}
    </div>
  )
})

export default ImageResultsEvent
