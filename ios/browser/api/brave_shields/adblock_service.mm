/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_service.h"

#include "base/strings/sys_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/ios/browser/api/brave_shields/adblock_filter_list+private.h"
#include "components/component_updater/component_updater_service.h"

@interface AdblockService () {
  component_updater::ComponentUpdateService* _cus;  // NOT OWNED
}
@property(nonatomic, copy) NSString* shieldsInstallPath;
@property(nonatomic, copy) NSArray<AdblockFilterList*>* regionalFilterLists;
@end

@implementation AdblockService

- (instancetype)initWithComponentUpdater:
    (component_updater::ComponentUpdateService*)componentUpdaterService {
  if ((self = [super init])) {
    _cus = componentUpdaterService;
  }
  return self;
}

- (void)registerDefaultShieldsComponent {
  __weak auto weakSelf = self;
  brave_shields::RegisterAdBlockDefaultComponent(
      _cus, base::BindRepeating(^(const base::FilePath& install_path) {
        [weakSelf adblockComponentDidBecomeReady:install_path];
      }));
}

- (void)adblockComponentDidBecomeReady:(const base::FilePath&)install_path {
  auto component_path = install_path;

  // Update shields install path (w/ KVO)
  [self willChangeValueForKey:@"shieldsInstallPath"];
  self.shieldsInstallPath = base::SysUTF8ToNSString(install_path.value());
  [self didChangeValueForKey:@"shieldsInstallPath"];

  // Get filter lists from catalog
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path.AppendASCII("regional_catalog.json")),
      base::BindOnce(^(const std::string json) {
        auto catalog = brave_shields::RegionalCatalogFromJSON(json);

        NSMutableArray* lists = [[NSMutableArray alloc] init];
        for (const auto& list : catalog) {
          [lists addObject:[[AdblockFilterList alloc]
                               initWithFilterList:adblock::FilterList(list)]];
        }
        self.regionalFilterLists = lists;

        if (self.shieldsComponentReady) {
          self.shieldsComponentReady(self.shieldsInstallPath);
        }
      }));
}

- (void)registerFilterListComponent:(AdblockFilterList*)filterList
                     componentReady:(void (^)(AdblockFilterList* filterList,
                                              NSString* _Nullable installPath))
                                        componentReady {
  brave_shields::RegisterAdBlockRegionalComponent(
      _cus, base::SysNSStringToUTF8(filterList.base64PublicKey),
      base::SysNSStringToUTF8(filterList.componentId),
      base::SysNSStringToUTF8(filterList.title),
      base::BindRepeating(^(const base::FilePath& install_path) {
        const auto installPath = base::SysUTF8ToNSString(install_path.value());
        componentReady(filterList, installPath);
      }));
}

- (void)unregisterFilterListComponent:(AdblockFilterList*)filterList {
  _cus->UnregisterComponent(base::SysNSStringToUTF8(filterList.componentId));
}

@end
