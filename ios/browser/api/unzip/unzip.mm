// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/unzip/unzip.h"

#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "components/services/unzip/in_process_unzipper.h"  // nogncheck
#include "components/services/unzip/public/cpp/unzip.h"
#include "ios/chrome/browser/shared/model/utils/rust_unzipper.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"

@implementation BraveUnzip

+ (void)unzip:(NSString*)zipFile
    toDirectory:(NSString*)directory
     completion:(void (^)(bool))completion {
  base::FilePath file_to_unzip = base::apple::NSStringToFilePath(zipFile);
  base::FilePath output_directory = base::apple::NSStringToFilePath(directory);

  scoped_refptr<base::SequencedTaskRunner> current_sequence =
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  current_sequence->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& file_to_unzip,
             const base::FilePath& output_directory, void (^completion)(bool)) {
            unzip::Unzip(
                unzip::LaunchInProcessUnzipper(), file_to_unzip,
                output_directory, unzip::mojom::UnzipOptions::New(),
                unzip::AllContents(), base::DoNothing(),
                base::BindOnce([](const base::FilePath& output_directory,
                                  void (^completion)(bool),
                                  bool success) { completion(success); },
                               output_directory, completion));
          },
          file_to_unzip, output_directory, completion));
}

+ (void)unzipData:(NSData*)data
       completion:(void (^)(NSArray<NSData*>* _Nullable unzippedFiles,
                            NSError* _Nullable error))completion {
  // Needs to be posted from a runner that can handle PostTaskAndReplyWithResult
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&UnzipData, data,
                                base::BindOnce(^(UnzipResultData results) {
                                  completion(results.unzipped_files,
                                             results.error);
                                })));
}

@end
