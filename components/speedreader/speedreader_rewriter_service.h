/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_REWRITER_SERVICE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_REWRITER_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"

namespace base {
class FilePathWatcher;
}

namespace speedreader {
class SpeedReader;
class Rewriter;
}  // namespace speedreader

class GURL;

namespace speedreader {

class SpeedreaderRewriterService {
 public:
  SpeedreaderRewriterService();
  ~SpeedreaderRewriterService();

  SpeedreaderRewriterService(const SpeedreaderRewriterService&) = delete;
  SpeedreaderRewriterService& operator=(const SpeedreaderRewriterService&) =
      delete;

  // The API
  bool URLLooksReadable(const GURL& url);
  std::unique_ptr<Rewriter> MakeRewriter(const GURL& url,
                                         const std::string& theme,
                                         const std::string& font_family,
                                         const std::string& font_size,
                                         const std::string& column_width);
  const std::string& GetContentStylesheet();

 private:
  void OnFileChanged(const base::FilePath& path, bool error);
  void OnWatcherStarted(base::FilePathWatcher* file_watcher);
  void OnLoadStylesheet(std::string stylesheet);

  scoped_refptr<base::SequencedTaskRunner> watch_task_runner_;
  raw_ptr<base::FilePathWatcher> file_watcher_ = nullptr;

  std::string content_stylesheet_;
  base::FilePath stylesheet_override_path_;
  std::unique_ptr<speedreader::SpeedReader> speedreader_;
  base::WeakPtrFactory<SpeedreaderRewriterService> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_REWRITER_SERVICE_H_
