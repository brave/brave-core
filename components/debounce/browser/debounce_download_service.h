/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_DOWNLOAD_SERVICE_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_DOWNLOAD_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/debounce/browser/debounce_service.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

class DebounceBrowserTest;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace debounce {

extern const char kDebounceConfigFile[];
extern const char kDebounceConfigFileVersion[];

enum DebounceAction {
  kDebounceNoAction,
  kDebounceRedirectToParam,
  kDebounceBase64DecodeAndRedirectToParam
};

class DebounceRule {
 public:
  DebounceRule();
  ~DebounceRule();

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<DebounceRule>* converter);
  static bool ParseDebounceAction(base::StringPiece value,
                                  DebounceAction* field);
  static bool GetURLPatternSetFromValue(const base::Value* value,
                                        extensions::URLPatternSet* result);

  bool Apply(const GURL& original_url,
             GURL* final_url);
  void clear();

  extensions::URLPatternSet include_pattern_set_;
  extensions::URLPatternSet exclude_pattern_set_;
  DebounceAction action_;
  std::string param_;
};

// The debounce download service is in charge
// of loading and parsing the debounce configuration file
class DebounceDownloadService : public LocalDataFilesObserver {
 public:
  explicit DebounceDownloadService(
      LocalDataFilesService* local_data_files_service);
  ~DebounceDownloadService() override;

  std::vector<std::unique_ptr<DebounceRule>>* rules();
  base::flat_set<std::string>* host_cache();

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // implementation of our own observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRulesReady(DebounceDownloadService* download_service) = 0;
  };
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  friend class ::DebounceBrowserTest;

  void OnDATFileDataReady(std::string contents);
  void LoadOnTaskRunner();
  void LoadDirectlyFromResourcePath();

  base::ObserverList<Observer> observers_;
  std::vector<std::unique_ptr<DebounceRule>> rules_;
  base::flat_set<std::string> host_cache_;
  base::FilePath resource_dir_;

  base::WeakPtrFactory<DebounceDownloadService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DebounceDownloadService);
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_DOWNLOAD_SERVICE_H_
