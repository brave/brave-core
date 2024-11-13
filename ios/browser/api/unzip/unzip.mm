// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/unzip/unzip.h"

#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "components/services/unzip/in_process_unzipper.h"  // nogncheck
#include "components/services/unzip/public/cpp/unzip.h"
#include "net/base/apple/url_conversions.h"

@implementation BraveUnzip

+ (void)unzip:(NSString*)zipFile
    toDirectory:(NSString*)directory
     completion:(void (^)(bool))completion {
  base::FilePath file_to_unzip = base::apple::NSStringToFilePath(zipFile);
  base::FilePath output_directory = base::apple::NSStringToFilePath(directory);

  unzip::Unzip(unzip::LaunchInProcessUnzipper(), file_to_unzip,
               output_directory, unzip::mojom::UnzipOptions::New(),
               unzip::AllContents(), base::DoNothing(),
               base::BindOnce([](const base::FilePath& output_directory,
                                 void (^completion)(bool),
                                 bool success) { completion(success); },
                              output_directory, completion));
}

@end
