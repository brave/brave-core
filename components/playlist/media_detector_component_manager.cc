/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/media_detector_component_manager.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/media_detector_component_installer.h"
#include "url/gurl.h"

namespace playlist {

namespace {

base::FilePath GetScriptPath(const base::FilePath& install_path) {
  return install_path.AppendASCII("index.js");
}

std::string ReadScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

}  // namespace

MediaDetectorComponentManager::MediaDetectorComponentManager(
    component_updater::ComponentUpdateService* component_update_service)
    : component_update_service_(component_update_service) {
  // TODO(sko) This list should be dynamically updated from the playlist.
  // Once it's done, remove this line.
  SetUseLocalListToHideMediaSrcAPI();

  SetUseLocalScriptForTesting();
}

MediaDetectorComponentManager::~MediaDetectorComponentManager() = default;

void MediaDetectorComponentManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void MediaDetectorComponentManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void MediaDetectorComponentManager::RegisterIfNeeded() {
  if (register_requested_)
    return;

  register_requested_ = true;
  RegisterMediaDetectorComponent(
      component_update_service_,
      base::BindRepeating(&MediaDetectorComponentManager::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnComponentReady(
    const base::FilePath& install_path) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadScript, GetScriptPath(install_path)),
      base::BindOnce(&MediaDetectorComponentManager::OnGetScript,
                     weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnGetScript(const std::string& script) {
  if (script.empty()) {
    LOG(ERROR) << __FUNCTION__ << " script is empty!";
    return;
  }

  script_ = script;

  for (auto& observer : observer_list_)
    observer.OnScriptReady(script_);
}

void MediaDetectorComponentManager::SetUseLocalScriptForTesting() {
  register_requested_ = true;
  // This script is modified version of
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/Playlist.js
  static const std::string kScript = R"-(
(function() {
  function is_nan(value) {
      return typeof value === "number" && value !== value;
  }

  function is_infinite(value) {
      return typeof value === "number" && (value === Infinity || value === -Infinity);
  }

  function clamp_duration(value) {
      if (is_nan(value)) {
          return 0.0;
      }

      if (is_infinite(value)) {
          return Number.MAX_VALUE;
      }
      return value;
  }

  // Algorithm:
  // Generate a random number from 0 to 256
  // Roll-Over clamp to the range [0, 15]
  // If the index is 13, set it to 4.
  // If the index is 17, clamp it to [0, 3]
  // Subtract that number from 15 (XOR) and convert the result to hex.
  function uuid_v4() {
      // X >> 2 = X / 4 (integer division)

      // AND-ing (15 >> 0) roll-over clamps to 15
      // AND-ing (15 >> 2) roll-over clamps to 3
      // So '8' digit is clamped to 3 (inclusive) and all others clamped to 15 (inclusive).

      // 0 XOR 15 = 15
      // 1 XOR 15 = 14
      // 8 XOR 15 = 7
      // So N XOR 15 = 15 - N

      // UUID string format generated with array appending
      // Results in "10000000-1000-4000-8000-100000000000".replace(...)
      return ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, (X) => {
          return (X ^ (crypto.getRandomValues(new Uint8Array(1))[0] & (15 >> (X >> 2)))).toString(16);
      });
  }

  function tagNode(node) {
      if (node) {
          if (!node.tagUUID) {
              node.tagUUID = uuid_v4();
              node.addEventListener('webkitpresentationmodechanged', (e) => e.stopPropagation(), true);
          }
      }
  }

  function getNodeData(node) {
    var src = node.src
    var mimeType = node.type
    var name = node.title;
    if (name == null || typeof name == 'undefined' || name == "") {
      // Try getting mobile youtube specific data
      name = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.title
    }

    if (name == null || typeof name == 'undefined' || name == "")
      name = document.title

    if (mimeType == null || typeof mimeType == 'undefined' || mimeType == "") {
      if (node.constructor.name == 'HTMLVideoElement') {
        mimeType = 'video';
      }

      if (node.constructor.name == 'HTMLAudioElement') {
        mimeType = 'audio';
      }

      if (node.constructor.name == 'HTMLSourceElement') {
        videoNode = node.closest('video');
        if (videoNode != null && typeof videoNode != 'undefined') {
          mimeType = 'video'
        } else {
          mimeType = 'audio'
        }
      }
    }

    if (src && src !== "") {
      tagNode(node);
      return [{
        "name": name,
        "src": src,
        "pageSrc": window.location.href,
        "pageTitle": document.title,
        "mimeType": mimeType,
        "duration": clamp_duration(node.duration),
        "detected": true,
        "tagId": node.tagUUID,
      }];
    } else {
      let target = node;
      let sources = []
      document.querySelectorAll('source').forEach(function(node) {
        if (node.src !== "") {
          if (node.closest('video') === target) {
            tagNode(target);
            sources.push({
              "name": name,
              "src": node.src,
              "pageSrc": window.location.href,
              "pageTitle": document.title,
              "mimeType": mimeType,
              "duration": clamp_duration(target.duration),
              "detected": true,
              "tagId": target.tagUUID,
            });
          }

          if (node.closest('audio') === target) {
            tagNode(target);
            sources.push({
              "name": name,
              "src": node.src,
              "pageSrc": window.location.href,
              "pageTitle": document.title,
              "mimeType": mimeType,
              "duration": clamp_duration(target.duration),
              "detected": true,
              "tagId": target.tagUUID,
            });
          }
        }
        
      });
      return sources;
    }
  }

  function getAllVideoElements() {
    return document.querySelectorAll('video');
  }

  function getAllAudioElements() {
    return document.querySelectorAll('audio');
  }

  function getThumbnail() {
    let thumbnail = document.querySelector('meta[property="og:image"]')?.content;
    if (thumbnail && !thumbnail.startsWith('http')) {
      thumbnail = new URL(thumbnail, location.origin).href
    }
    
    if (thumbnail && thumbnail !== '') return thumbnail;

    // Try getting mobile youtube specific data
    thumbnail = window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.thumbnail?.thumbnails;
    if (thumbnail && Array.isArray(thumbnail)) {
      thumbnail = thumbnail[0]?.url
    }

    return thumbnail
  }

  function getAuthor() {
    // Try getting mobile youtube specific data
    return window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.author
  }

  function getDurationInSeconds() {
    // Try getting mobile youtube specific data
    return window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.lengthSeconds
  }

  let videoElements = getAllVideoElements() ?? [];
  let audioElements = getAllAudioElements() ?? [];
  // TODO(sko) These data could be incorrect when there're multiple items.
  // For now we're assuming that the first media is a representative one.
  const thumbnail = getThumbnail();
  const author = getAuthor();
  const durationInSeconds = getDurationInSeconds();

  let medias = []
  videoElements.forEach(e => medias = medias.concat( getNodeData(e)));
  audioElements.forEach(e => medias = medias.concat( getNodeData(e)));

  if (medias.length) {
    medias[0].thumbnail = thumbnail;
    medias[0].author = author;
    if (!medias[0].duration)
     medias[0].duration = durationInSeconds;
  }

  return medias;
})();
  )-";

  OnGetScript(kScript);
}

bool MediaDetectorComponentManager::ShouldHideMediaSrcAPI(
    const GURL& url) const {
  net::SchemefulSite schemeful_site(url);
  return base::ranges::any_of(sites_to_hide_media_src_api_,
                              [&schemeful_site](const auto& site_to_hide) {
                                return site_to_hide == schemeful_site;
                              });
}

void MediaDetectorComponentManager::SetUseLocalListToHideMediaSrcAPI() {
  sites_to_hide_media_src_api_ = {
      {net::SchemefulSite(GURL("https://youtube.com"))}};
}

}  // namespace playlist
