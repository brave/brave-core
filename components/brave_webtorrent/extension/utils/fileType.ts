// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const SUPPORTED_VIDEO_EXTENSIONS = [
  'm4v',
  'mkv',
  'mov',
  'mp4',
  'ogv',
  'webm'
]

const SUPPORTED_AUDIO_EXTENSIONS = [
  'aac',
  'flac',
  'm4a',
  'm4b',
  'm4p',
  'mp3',
  'oga',
  'ogg',
  'wav'
]

const SUPPORTED_IMAGE_EXTENSIONS = [
  'bmp',
  'gif',
  'jpeg',
  'jpg',
  'png',
  'svg'
]

const SUPPORTED_PDF_EXTENSIONS = [
  'pdf'
]

const SUPPORTED_IFRAME_EXTENSIONS = [
  'css',
  'html',
  'js',
  'md',
  'txt'
]

export type FileType = 'video' | 'audio' | 'image' | 'pdf' | 'iframe' | 'unknown'
export const mediaTypes: FileType[] = ['video', 'audio', 'image']
export const isMedia = (fileType: FileType) => mediaTypes.includes(fileType)
// Given 'foo.txt', returns 'txt'
// undefined or README return ''
export const getExtension = (filename?: string) => {
  if (!filename) return ''
  const ix = filename.lastIndexOf('.')
  if (ix < 0) return ''
  return filename.substring(ix + 1)
}

export const getFileType = (file?: { name: string }): FileType => {
  const extension = getExtension(file?.name)
  if (SUPPORTED_VIDEO_EXTENSIONS.includes(extension)) return 'video'
  if (SUPPORTED_AUDIO_EXTENSIONS.includes(extension)) return 'audio'
  if (SUPPORTED_IMAGE_EXTENSIONS.includes(extension)) return 'image'
  if (SUPPORTED_PDF_EXTENSIONS.includes(extension)) return 'pdf'
  if (SUPPORTED_IFRAME_EXTENSIONS.includes(extension)) return 'iframe'
  return 'unknown'
}
