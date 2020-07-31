/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/brave_widevine_bundle_unzipper.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "components/services/unzip/content/unzip_service.h"
#include "components/services/unzip/public/cpp/unzip.h"
#include "content/public/browser/browser_thread.h"
#include "media/cdm/cdm_paths.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

namespace {
// This is filter function for unzipper. We only unzip cdm library.
// Returns true if |file_path| widevine cdm library name.
bool IsWidevineCdmFile(const base::FilePath& file_path) {
  CHECK(!file_path.IsAbsolute());
  return base::FilePath::CompareEqualIgnoreCase(
             file_path.value(),
             base::GetNativeLibraryName(kWidevineCdmLibraryName)) ||
         base::FilePath::CompareEqualIgnoreCase(file_path.value(),
                                                "manifest.json");
}

base::Optional<base::FilePath> GetTempDirForUnzip() {
  base::FilePath unzip_dir;
  if (!base::CreateNewTempDirectory(base::FilePath::StringType(), &unzip_dir))
    return base::Optional<base::FilePath>();

  return unzip_dir;
}

}  // namespace

// static
scoped_refptr<BraveWidevineBundleUnzipper>
BraveWidevineBundleUnzipper::Create(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner,
    DoneCallback done_callback) {
  DCHECK(file_task_runner);
  DCHECK(done_callback);
  return base::WrapRefCounted(
      new BraveWidevineBundleUnzipper(file_task_runner,
                                      std::move(done_callback)));
}

void BraveWidevineBundleUnzipper::LoadFromZipFileInDir(
    const base::FilePath& zipped_bundle_file,
    const base::FilePath& unzip_dir,
    bool delete_file) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(!zipped_bundle_file.empty());
  DCHECK(!unzip_dir.empty());
  DVLOG(1) << __func__ << ": zipped bundle file: " << zipped_bundle_file;
  DVLOG(1) << __func__ << ": target install dir: " << unzip_dir;

  delete_zip_file_ = delete_file;
  target_unzip_dir_ = unzip_dir;
  zipped_bundle_file_ = zipped_bundle_file;

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(&GetTempDirForUnzip),
      base::BindOnce(&BraveWidevineBundleUnzipper::OnGetTempDirForUnzip, this));
}

void BraveWidevineBundleUnzipper::OnGetTempDirForUnzip(
    base::Optional<base::FilePath> temp_unzip_dir) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!temp_unzip_dir) {
    UnzipDone("Getting temp dir for unzip failed");
    return;
  }

  temp_unzip_dir_ = *temp_unzip_dir;

  DVLOG(1) << __func__ << ": temp unzip dir: " << temp_unzip_dir_;

  unzip::UnzipWithFilter(
      unzip::LaunchUnzipper(), zipped_bundle_file_, *temp_unzip_dir,
      base::BindRepeating(&IsWidevineCdmFile),
      base::BindOnce(&BraveWidevineBundleUnzipper::OnUnzippedInTempDir, this));
}

void BraveWidevineBundleUnzipper::OnUnzippedInTempDir(bool status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!status) {
    UnzipDone("Unzip failed");
    return;
  }
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(
          &BraveWidevineBundleUnzipper::MoveUnzippedLibFromTempToTargetDir,
          this),
      base::BindOnce(&BraveWidevineBundleUnzipper::UnzipDone, this));
}

std::string BraveWidevineBundleUnzipper::MoveUnzippedLibFromTempToTargetDir() {
  // Lib should go into the platform specific directory, whereas the manifest
  // goes to the top directory.
  const base::FilePath widevine_lib_dir =
      media::GetPlatformSpecificDirectory(target_unzip_dir_);
  base::CreateDirectory(widevine_lib_dir);
  const std::string widevine_lib_name =
      base::GetNativeLibraryName(kWidevineCdmLibraryName);

  const std::map<const base::StringPiece, const base::FilePath> files = {
      {widevine_lib_name, widevine_lib_dir},
      {"manifest.json", target_unzip_dir_}};

  std::string error;
  for (auto const& file : files) {
    const base::FilePath source = temp_unzip_dir_.AppendASCII(file.first);
    DCHECK(base::PathExists(source));
    const base::FilePath target = file.second.AppendASCII(file.first);

    if (!base::Move(source, target)) {
      error = std::string("widevine lib failed to move: ") +
        source.value() + " to " + target.value();
      break;
    }
  }

  base::DeletePathRecursively(temp_unzip_dir_);

  if (delete_zip_file_)
    base::DeleteFile(zipped_bundle_file_);

  return error;
}

void BraveWidevineBundleUnzipper::UnzipDone(const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::move(done_callback_).Run(error);
}

BraveWidevineBundleUnzipper::BraveWidevineBundleUnzipper(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner,
    DoneCallback done_callback)
    : file_task_runner_(file_task_runner),
      done_callback_(std::move(done_callback)) {
}

BraveWidevineBundleUnzipper::~BraveWidevineBundleUnzipper() {
}
