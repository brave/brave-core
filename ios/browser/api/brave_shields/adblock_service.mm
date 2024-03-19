/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_service.h"

#include "base/strings/sys_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/ios/browser/api/brave_shields/adblock_filter_list_catalog_entry+private.h"
#include "components/component_updater/component_updater_service.h"

@interface AdblockService () {
  component_updater::ComponentUpdateService* _cus;  // NOT OWNED
}
@end

@implementation AdblockService

- (instancetype)initWithComponentUpdater:
    (component_updater::ComponentUpdateService*)componentUpdaterService {
  if ((self = [super init])) {
    _cus = componentUpdaterService;
  }
  return self;
}

- (void)registerResourceComponent:
    (void (^)(NSString* _Nullable installPath))componentReady {
  brave_shields::RegisterAdBlockDefaultResourceComponent(
      _cus, base::BindRepeating(^(const base::FilePath& install_path) {
        const auto installPath = base::SysUTF8ToNSString(install_path.value());
        componentReady(installPath);
      }));
}

- (void)registerFilterListCatalogComponent:
    (void (^)(NSArray<AdblockFilterListCatalogEntry*>* filterLists))
        componentReady {
  brave_shields::RegisterAdBlockFilterListCatalogComponent(
      _cus, base::BindRepeating(^(const base::FilePath& install_path) {
        // Get filter lists from catalog
        base::ThreadPool::PostTaskAndReplyWithResult(
            FROM_HERE, {base::MayBlock()},
            base::BindOnce(&brave_component_updater::GetDATFileAsString,
                           install_path.AppendASCII("list_catalog.json")),
            base::BindOnce(^(const std::string& json) {
              // Parse data
              auto catalog = brave_shields::FilterListCatalogFromJSON(json);

              NSMutableArray* lists = [[NSMutableArray alloc] init];
              for (const auto& entry : catalog) {
                [lists addObject:[[AdblockFilterListCatalogEntry alloc]
                                     initWithFilterListCatalogEntry:
                                         brave_shields::FilterListCatalogEntry(
                                             entry)]];
              }

              componentReady(lists);
            }));
      }));
}

- (void)registerFilterListComponent:(AdblockFilterListCatalogEntry*)entry
                     componentReady:(void (^)(NSString* _Nullable installPath))
                                        componentReady {
  brave_shields::RegisterAdBlockFiltersComponent(
      _cus, base::SysNSStringToUTF8(entry.base64PublicKey),
      base::SysNSStringToUTF8(entry.componentId),
      base::SysNSStringToUTF8(entry.title),
      base::BindRepeating(^(const base::FilePath& install_path) {
        const auto installPath = base::SysUTF8ToNSString(install_path.value());
        componentReady(installPath);
      }));
}

- (void)unregisterFilterListComponent:(AdblockFilterListCatalogEntry*)entry {
  _cus->UnregisterComponent(base::SysNSStringToUTF8(entry.componentId));
}

@end
