/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/media_detector_component_manager.h"

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/media_detector_component_installer.h"
#include "components/grit/brave_components_resources.h"
#include "net/base/url_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace playlist {

namespace {

using ScriptName = base::FilePath::StringType;
using ScriptToSchemefulSiteMap = base::flat_map<ScriptName, net::SchemefulSite>;
using ScriptToResourceIdMap = base::flat_map<ScriptName, int>;

const base::FilePath::StringType& GetBaseScriptName() {
  static const base::NoDestructor base_script(
      ScriptName(FILE_PATH_LITERAL("index.js")));
  return *base_script;
}

const ScriptToSchemefulSiteMap& GetScriptNameToSchemefulSiteMap() {
  static const base::NoDestructor script_name_to_schemeful_sites(
      ScriptToSchemefulSiteMap{
          {FILE_PATH_LITERAL("youtube.com.js"),
           net::SchemefulSite(GURL("https://youtube.com"))}});

  return *script_name_to_schemeful_sites;
}

base::flat_map<ScriptName, std::string> GetLocalScriptMap() {
  const auto& rb = ui::ResourceBundle::GetSharedInstance();
  return {
      {GetBaseScriptName(),
       std::string(rb.LoadDataResourceString(IDR_PLAYLIST_MEDIA_DETECTOR_JS))},
      {FILE_PATH_LITERAL("youtube.com.js"),
       std::string(
           rb.LoadDataResourceString(IDR_PLAYLIST_MEDIA_DETECTOR_YOUTUBE_JS))},
  };
}

std::string ReadScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

base::flat_map<ScriptName, std::string> ReadScriptsFromComponent(
    base::flat_set<base::FilePath> files) {
  base::flat_map<ScriptName, std::string> script_map;
  for (const auto& path : files) {
    if (auto script = ReadScript(path); !script.empty()) {
      script_map[path.BaseName().value()] = script;
    }
  }

  return script_map;
}

std::vector<std::string> SplitString(std::string_view input,
                                     std::string_view delim) {
  return base::SplitString(input, delim, base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

std::vector<std::string> SplitPaths(const GURL& url) {
  return SplitString(url.path_piece(), "/");
}

}  // namespace

MediaDetectorComponentManager::MediaDetectorComponentManager(
    component_updater::ComponentUpdateService* component_update_service)
    : component_update_service_(component_update_service) {
  // TODO(sko) These lists should be dynamically updated from the playlist.
  // Even after we finish the job, we should leave these call so that we can
  // use local resources until the component is updated.
  SetUseLocalListToHideMediaSrcAPI();
  SetUseLocalListToUseFakeUA();

  site_and_media_page_url_checkers_ = {
      {net::SchemefulSite(GURL("https://youtube.com")),
       // youtube.com/watch?v=123123
       base::BindRepeating([](const GURL& url) {
         if (url.path_piece() != "/watch" || !url.has_query()) {
           return false;
         }

         std::string video_id;
         net::GetValueForKeyInQuery(url, "v", &video_id);
         return !video_id.empty();
       })},
      {net::SchemefulSite(GURL("https://bbcgoodfood.com")),
       base::BindRepeating([](const GURL& url) {
         // "https://bbcgoodfood.com/videos/foo"
         if (url.path_piece() == "/") {
           return false;
         }

         auto paths = SplitPaths(url);
         return paths.size() == 2 && paths.front() == "videos";
       })},
      {net::SchemefulSite(GURL("https://bitchute.com")),
       base::BindRepeating([](const GURL& url) {
         // https://www.bitchute.com/video/XXXXXXXXXXXX/
         if (url.path_piece() == "/") {
           return false;
         }

         auto paths = SplitPaths(url);
         return paths.size() == 2 && paths.front() == "video";
       })},
      {net::SchemefulSite(GURL("https://brighteon.com")),
       base::BindRepeating([](const GURL& url) {
         // GUID format
         // https://www.brighteon.com/XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
         if (url.path_piece() == "/") {
           return false;
         }

         auto paths = SplitPaths(url);
         if (paths.size() != 1) {
           return false;
         }

         const auto guid_parts = SplitString(paths.front(), "-");
         if (guid_parts.size() != 5) {
           return false;
         }

         return guid_parts.at(0).length() == 8 &&
                guid_parts.at(1).length() == 4 &&
                guid_parts.at(2).length() == 4 &&
                guid_parts.at(3).length() == 4 &&
                guid_parts.at(4).length() == 12;
       })},
      {net::SchemefulSite(GURL("https://marthastewart.com")),
       base::BindRepeating([](const GURL& url) {
         // TODO(sko) Can't tell a page would have media from url.
         return false;
       })},
      {net::SchemefulSite(GURL("https://rumble.com/")),
       base::BindRepeating([](const GURL& url) {
         // Starts with vXXXXXX (6 chars) and followed by title of video
         // https://rumble.com/vXXXXXX-foo-bar-baz.html
         if (url.path_piece() == "/") {
           return false;
         }

         auto paths = SplitPaths(url);
         if (paths.size() != 1) {
           return false;
         }

         const auto media_page_parts = SplitString(paths.front(), "-");
         return media_page_parts.size() > 1 &&
                media_page_parts.front().size() == 7 &&
                media_page_parts.front().at(0) == 'v';
       })},
      {net::SchemefulSite(GURL("https://ted.com")),
       base::BindRepeating([](const GURL& url) {
         // https://www.ted.com/talks/frances_s_chance_are_insect_brains_the_secret_to_great_ai
         if (url.path_piece() == "/") {
           return false;
         }

         auto paths = SplitPaths(url);
         return paths.size() == 2 && paths.front() == "talks";
       })},
      {net::SchemefulSite(GURL("https://vimeo.com")),
       base::BindRepeating([](const GURL& url) {
         // https://vimeo.com/1234567  (5-9 digits)
         return false;
       })},
  };
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
  base::flat_set<base::FilePath> files(
      {install_path.Append(GetBaseScriptName())});
  for (const auto& [file, _] : GetScriptNameToSchemefulSiteMap()) {
    files.insert(install_path.Append(file));
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadScriptsFromComponent, files),
      base::BindOnce(&MediaDetectorComponentManager::OnGetScripts,
                     weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnGetScripts(
    const MediaDetectorComponentManager::ScriptMap& script_map) {
  if (script_map.empty()) {
    LOG(ERROR) << __FUNCTION__ << " scripts are empty!";
    return;
  }

  DCHECK(script_map.count(GetBaseScriptName()));
  base_script_ = script_map.at(GetBaseScriptName());

  // This could have been filled when we've used media detector script before
  // component updater finishes its work.
  site_specific_detectors_.clear();

  const auto& schemeful_site_map = GetScriptNameToSchemefulSiteMap();
  for (const auto& [script_name, script] : script_map) {
    if (schemeful_site_map.count(script_name)) {
      site_specific_detectors_[schemeful_site_map.at(script_name)] = script;
    }
  }

  for (auto& observer : observer_list_)
    observer.OnScriptReady(base_script_);
}

void MediaDetectorComponentManager::SetUseLocalScriptForTesting() {
  register_requested_ = true;

  OnGetScripts(GetLocalScriptMap());
}

bool MediaDetectorComponentManager::ShouldHideMediaSrcAPI(
    const GURL& url) const {
  net::SchemefulSite schemeful_site(url);
  return base::ranges::any_of(sites_to_hide_media_src_api_,
                              [&schemeful_site](const auto& site_to_hide) {
                                return site_to_hide == schemeful_site;
                              });
}

std::string MediaDetectorComponentManager::GetMediaDetectorScript(
    const GURL& url) {
  if (base_script_.empty()) {
    // In case we have yet to fetch the script, use local script instead. At the
    // same time, fetch the script from component.
    RegisterIfNeeded();
    OnGetScripts(GetLocalScriptMap());
  }

  std::string detector_script = base_script_;
  DCHECK(!detector_script.empty());

  if (net::SchemefulSite site(url); site_specific_detectors_.count(site)) {
    constexpr std::string_view kPlaceholder =
        "const siteSpecificDetector = null";
    auto pos = detector_script.find(kPlaceholder);
    if (pos != std::string::npos) {
      detector_script.replace(pos, kPlaceholder.length(),
                              site_specific_detectors_.at(site));
    } else {
      // Reportedly, in some environments(e.g. Android release), the js resource
      // could be minified by removing white spaces.
      constexpr std::string_view kPlaceholderWithoutWhitespace =
          "const siteSpecificDetector=null";
      pos = detector_script.find(kPlaceholderWithoutWhitespace);
      if (pos != std::string::npos) {
        detector_script.replace(pos, kPlaceholderWithoutWhitespace.length(),
                                site_specific_detectors_.at(site));
      } else {
        LOG(ERROR) << "Couldn't find `const siteSpecificDetector = null` from "
                      "base script";
      }
    }
  }

  return detector_script;
}

void MediaDetectorComponentManager::SetUseLocalListToHideMediaSrcAPI() {
  sites_to_hide_media_src_api_ = {
      {net::SchemefulSite(GURL("https://youtube.com"))},
      {net::SchemefulSite(GURL("https://vimeo.com"))},
      {net::SchemefulSite(GURL("https://ted.com"))},
      {net::SchemefulSite(GURL("https://bitchute.com"))},
      {net::SchemefulSite(GURL("https://marthastewart.com"))},
      {net::SchemefulSite(GURL("https://bbcgoodfood.com"))},
      {net::SchemefulSite(GURL("https://rumble.com/"))},
      {net::SchemefulSite(GURL("https://brighteon.com"))},
  };
}

bool MediaDetectorComponentManager::ShouldUseFakeUA(const GURL& url) const {
  net::SchemefulSite schemeful_site(url);
  return base::ranges::any_of(
      sites_to_use_fake_ua_,
      [&schemeful_site](const auto& site_to_use_fake_ua) {
        return site_to_use_fake_ua == schemeful_site;
      });
}

void MediaDetectorComponentManager::SetUseLocalListToUseFakeUA() {
  sites_to_use_fake_ua_ = {
      {net::SchemefulSite(GURL("https://ted.com"))},
      {net::SchemefulSite(GURL("https://marthastewart.com"))},
      {net::SchemefulSite(GURL("https://bbcgoodfood.com"))},
      {net::SchemefulSite(GURL("https://rumble.com/"))},
      {net::SchemefulSite(
          GURL("https://brighteon.com"))},  // This site partially supported,
                                            // Audio only.
  };
}

bool MediaDetectorComponentManager::CouldURLHaveMedia(const GURL& url) const {
  net::SchemefulSite site(url);
  if (!base::Contains(site_and_media_page_url_checkers_, site)) {
    return false;
  }

  return site_and_media_page_url_checkers_.at(site).Run(url);
}

}  // namespace playlist
