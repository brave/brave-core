// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import thumbnail from './thumbnail.png'

export const mockData : PlaylistMojo.Playlist[] = [
  {id: "default", name: "", items: [
    {
      id: "1",
      name: "Awesome Video!",
      pageSource: {url: "https://foo.com/"},
      pageRedirected: {url: "https://foo.com/"},
      mediaSource: {url: "https://foo.com/video.mp4"},
      mediaPath: {url: "https://foo.com/video.mp4"},
      thumbnailSource: {url: "https://foo.com/thumbnail.jpg"},
      thumbnailPath: {url: "https://foo.com/thumbnail.jpg"},
      cached: false,
      lastPlayedPosition: 0,
      author: "Creator",
      duration: "0",
      parents: ["default"],
    }
  ]},
  {id: "1", name: "Empty Playlist", items: [] },
  {id: "2", name: "My awesome list", items: [
    {
      id: "2",
      name: "Awesome Video!",
      pageSource: {url: "https://foo.com/"},
      pageRedirected: {url: "https://foo.com/"},
      mediaSource: {url: "https://foo.com/video.mp4"},
      mediaPath: {url: "https://foo.com/video.mp4"},
      thumbnailSource: {url: thumbnail},
      thumbnailPath: {url: thumbnail},
      cached: false,
      lastPlayedPosition: 0,
      author: "Creator",
      duration: "0",
      parents: ["2"],
    }
  ]},
]
