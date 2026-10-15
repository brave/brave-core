// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Evaluated on demand. `__braveWasPlaying` is set while the scene is resigning active, before WebKit pauses video for backgrounding.

function visitVideos(root, visit) {
  if (!root || !root.querySelectorAll) return
  var videos = root.querySelectorAll('video')
  for (var i = 0; i < videos.length; i++) {
    var video = videos[i]
    if (video) visit(video)
  }
  var nodes = root.querySelectorAll('*')
  for (var j = 0; j < nodes.length; j++) {
    if (nodes[j].shadowRoot) visitVideos(nodes[j].shadowRoot, visit)
  }
}

function markPlayingVideos() {
  visitVideos(document, function (video) {
    video.__braveWasPlaying = !video.paused && !video.ended
  })
}

function resumePlayingVideos() {
  function resume() {
    visitVideos(document, function (video) {
      if (video.ended || !video.__braveWasPlaying || !video.paused) return
      video.__braveWasPlaying = false
      video.play()
    })
  }
  resume()
  setTimeout(resume, 0)
  setTimeout(resume, 100)
}
