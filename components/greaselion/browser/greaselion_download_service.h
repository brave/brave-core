/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "content/public/browser/notification_types.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

class GreaselionServiceTest;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace greaselion {

extern const char kGreaselionConfigFile[];
extern const char kGreaselionConfigFileVersion[];

enum GreaselionPreconditionValue { kMustBeFalse, kMustBeTrue, kAny };

struct GreaselionPreconditions {
  GreaselionPreconditionValue rewards_enabled;
  GreaselionPreconditionValue twitter_tips_enabled;
};

class GreaselionRule {
 public:
  GreaselionRule();
  void Parse(base::DictionaryValue* preconditions_value,
             base::ListValue* urls_value,
             base::ListValue* scripts_value,
             const base::FilePath& root_dir,
             scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~GreaselionRule();

  bool Matches(const GURL& url, GreaselionFeatures state) const;
  void Populate(std::vector<std::string>* scripts) const;

  // implementation of observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRuleReady(GreaselionRule* rule,
                             bool all_scripts_loaded_successfully) = 0;
  };
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();
  void AddScriptAfterLoad(std::unique_ptr<std::string> contents, bool did_load);
  GreaselionPreconditionValue ParsePrecondition(base::DictionaryValue* root,
                                                const char* key);
  bool PreconditionFulfilled(GreaselionPreconditionValue precondition,
                             bool value) const;

  base::ObserverList<Observer> observers_;
  int pending_scripts_;
  bool all_scripts_loaded_successfully_;
  extensions::URLPatternSet urls_;
  std::vector<std::string> scripts_;
  GreaselionPreconditions preconditions_;
  base::WeakPtrFactory<GreaselionRule> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(GreaselionRule);
};

// The Greaselion download service is in charge
// of loading and parsing the Greaselion configuration file
// and the scripts that the configuration file references
class GreaselionDownloadService : public LocalDataFilesObserver,
                                  GreaselionRule::Observer {
 public:
  explicit GreaselionDownloadService(
      LocalDataFilesService* local_data_files_service);
  ~GreaselionDownloadService() override;

  std::vector<std::unique_ptr<GreaselionRule>>* rules();
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // implementation of GreaselionRule::Observer
  void OnRuleReady(GreaselionRule* rule,
                   bool all_scripts_loaded_successfully) override;

  // implementation of our own observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnAllScriptsLoaded(GreaselionDownloadService* download_service,
                                    bool success) = 0;
  };
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  friend class ::GreaselionServiceTest;

  void OnDATFileDataReady(std::string contents);
  void LoadOnTaskRunner();

  base::ObserverList<Observer> observers_;
  int pending_rules_;
  bool all_scripts_loaded_successfully_;
  std::vector<std::unique_ptr<GreaselionRule>> rules_;
  base::FilePath install_dir_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<GreaselionDownloadService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(GreaselionDownloadService);
};  // namespace greaselion

// Creates the GreaselionDownloadService
std::unique_ptr<GreaselionDownloadService> GreaselionDownloadServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_
