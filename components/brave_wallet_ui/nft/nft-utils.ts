// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

export type MultimediaType =
  | 'image'
  | 'audio'
  | 'video'
  | 'model'
  | 'html'
  | 'other'

export const getMediaType = (mediaMimeType: string): MultimediaType => {
  const mimeType = mediaMimeType.toLowerCase()
   if (mimeType.startsWith('image/')) {
     return 'image'
   } else if (mimeType.startsWith('audio/')) {
     return 'audio'
   } else if (mimeType.startsWith('video/')) {
     return 'video'
   } else if (mimeType.startsWith('model/')) {
     return 'model'
   } else if (mimeType === 'text/html') {
     return 'html'
   }

  return 'other'
}
