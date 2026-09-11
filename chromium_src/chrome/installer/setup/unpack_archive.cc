/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Upstream dropped support for differential updates when it moved that logic
// into Omaha 4. We are still on Omaha 3 on Windows and therefore need to keep
// it. See: github.com/brave/brave-core/pull/31937
//
// A compressed archive may therefore hold a patch file instead of chrome.7z,
// in which case the patch is applied to the installed version's chrome.7z. We
// also record the archive type and the path of the uncompressed archive in
// `installer_state` for later stages of the install.
//
// This file reimplements UnpackChromeArchive() rather than wrapping it, but it
// reuses upstream's helpers (archive resource extraction, unpacking) by
// including upstream's implementation with its entry point renamed out of the
// way.

#include "chrome/installer/setup/unpack_archive.h"

#include <string>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "base/version.h"
#include "brave/installer/setup/archive_patch_helper.h"
#include "brave/installer/setup/brave_setup_util.h"
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/installer_util_strings.h"
#include "chrome/installer/util/lzma_util.h"
#include "chrome/installer/util/util_constants.h"

namespace installer {

// Upstream's entry point, renamed by the #define below so that it does not
// clash with Brave's version. Nothing calls it, but keeping it compiled gives
// upstream's file-local helpers a user.
base::expected<void, InstallStatus> UnpackChromeArchive_ChromiumImpl(
    const base::FilePath& unpack_path,
    const base::FilePath& setup_exe,
    const base::CommandLine& cmd_line,
    const InstallerState& installer_state);

}  // namespace installer

// N.B.: The include of upstream's header above must come first so that the
// declaration of UnpackChromeArchive() is not renamed by this #define.
#define UnpackChromeArchive UnpackChromeArchive_ChromiumImpl
#include <chrome/installer/setup/unpack_archive.cc>
#undef UnpackChromeArchive

namespace installer {

namespace {

// Applies `patch_file`, which was extracted from a differential archive, to
// the installed version's chrome.7z, writing the result to `target`.
base::expected<void, InstallStatus> ApplyChromeArchivePatch(
    const base::FilePath& unpack_path,
    const base::CommandLine& cmd_line,
    const InstallerState& installer_state,
    const base::FilePath& patch_file,
    const base::FilePath& target) {
  base::Version previous_version;
  if (cmd_line.HasSwitch(switches::kPreviousVersion)) {
    previous_version =
        base::Version(cmd_line.GetSwitchValueASCII(switches::kPreviousVersion));
  }

  // Upstream no longer passes the installation state down to this point, so
  // read it from the registry here. This only happens for differential
  // updates.
  InstallationState original_state;
  original_state.Initialize();

  // Find the installed version's archive to serve as the source for patching.
  const base::FilePath patch_source =
      FindArchiveToPatch(original_state, installer_state, previous_version);
  if (patch_source.empty()) {
    LOG(ERROR) << "Failed to find archive to patch.";
    return base::unexpected(DIFF_PATCH_SOURCE_MISSING);
  }

  // UMA tells us the following about the time required for patching as of M75:
  // --- Foreground ---
  //   12s (50%ile) / 3-6m (99%ile)
  // --- Background ---
  //   1m (50%ile) / >60m (99%ile)
  installer_state.SetStage(PATCHING);
  ArchivePatchHelper patch_helper(
      unpack_path, /*compressed_archive=*/base::FilePath(), patch_source,
      target, UnPackConsumer::COMPRESSED_CHROME_ARCHIVE);
  // The patch file was already extracted by the caller.
  patch_helper.set_last_uncompressed_file(patch_file);
  if (!patch_helper.ApplyAndDeletePatch()) {
    return base::unexpected(APPLY_DIFF_PATCH_FAILED);
  }
  return base::ok();
}

// Brave's version of upstream's UnpackChromeArchiveImpl(), with the handling
// of differential archives restored.
base::expected<void, InstallStatus> BraveUnpackChromeArchiveImpl(
    const base::FilePath& unpack_path,
    const base::FilePath& setup_exe,
    const base::CommandLine& cmd_line,
    InstallerState& installer_state) {
  base::FilePath mini_installer_path =
      cmd_line.GetSwitchValuePath(switches::kMiniInstallerPath);
  base::FilePath install_archive =
      cmd_line.GetSwitchValuePath(switches::kInstallArchive);
  base::FilePath uncompressed_archive =
      cmd_line.GetSwitchValuePath(switches::kUncompressedArchive);

  installer_state.archive_type = UNKNOWN_ARCHIVE_TYPE;
  installer_state.uncompressed_archive.clear();

  // Whether `uncompressed_archive` below is the result of expanding a
  // compressed archive, in which case it may be a patch rather than chrome.7z.
  bool from_compressed_archive = false;

  if (!mini_installer_path.empty()) {
    // Mode 1: Resource embedded in mini_installer.exe.
    // --install-archive and --uncompressed-archive are incompatible with
    // --mini-installer-path.
    CHECK(install_archive.empty() && uncompressed_archive.empty());

    std::wstring resource_type =
        cmd_line.GetSwitchValueNative(switches::kArchiveResourceType);
    const bool is_compressed_archive = IsCompressedResourceType(resource_type);
    installer_state.SetStage(is_compressed_archive ? UNCOMPRESSING : UNPACKING);

    if (UnpackFromMiniInstaller(
            mini_installer_path,
            cmd_line.GetSwitchValueNative(switches::kArchiveResourceName),
            resource_type, unpack_path,
            uncompressed_archive) != UNPACK_NO_ERROR) {
      return base::unexpected(is_compressed_archive ? UNCOMPRESSION_FAILED
                                                    : UNPACKING_FAILED);
    }

    if (uncompressed_archive.empty()) {
      if (is_compressed_archive) {
        LOG(ERROR) << "Failed to uncompress an archive from resource "
                   << cmd_line.GetSwitchValueNative(
                          switches::kArchiveResourceName)
                   << " in file " << mini_installer_path;
        return base::unexpected(INVALID_ARCHIVE);
      }
      // Directly unpacked uncompressed resource. There is no archive on disk
      // to keep around for a future differential update.
      installer_state.archive_type = FULL_ARCHIVE_TYPE;
      return base::ok();
    }
    from_compressed_archive = true;
  } else {
    // Mode 2: Archive files on disk.
    // At most one of --install-archive and --uncompressed-archive may be
    // provided.
    CHECK(install_archive.empty() || uncompressed_archive.empty());

    if (!install_archive.empty()) {
      // --install-archive was given: uncompress then unpack.
      installer_state.SetStage(UNCOMPRESSING);

      VLOG(1) << "Installing Brave from compressed archive " << install_archive;
      if (Unpack(UnPackConsumer::COMPRESSED_CHROME_ARCHIVE, install_archive,
                 unpack_path, &uncompressed_archive) != UNPACK_NO_ERROR) {
        return base::unexpected(UNCOMPRESSION_FAILED);
      }
      if (uncompressed_archive.empty()) {
        LOG(ERROR) << "Failed to uncompress an archive from "
                   << install_archive;
        return base::unexpected(INVALID_ARCHIVE);
      }
      from_compressed_archive = true;
    } else if (uncompressed_archive.empty()) {
      // Neither --install-archive nor --uncompressed-archive was given. Try
      // unpacking chrome.7z next to this executable.
      installer_state.SetStage(UNPACKING);
      const base::FilePath chrome_archive =
          setup_exe.DirName().Append(kChromeArchive);
      UnPackStatus status = Unpack(UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE,
                                   chrome_archive, unpack_path, nullptr);
      if (status == UNPACK_NO_ERROR) {
        installer_state.archive_type = FULL_ARCHIVE_TYPE;
        installer_state.uncompressed_archive = chrome_archive;
        return base::ok();  // Success.
      }
      if (status != UNPACK_ARCHIVE_NOT_FOUND) {
        return base::unexpected(UNPACKING_FAILED);
      }
    }  // else --uncompressed-archive was given.

    if (uncompressed_archive.empty()) {
      // Neither --install-archive nor --uncompressed-archive was given and
      // chrome.7z wasn't found. Try uncompressing chrome.packed.7z next to this
      // executable.
      installer_state.SetStage(UNCOMPRESSING);
      if (Unpack(UnPackConsumer::COMPRESSED_CHROME_ARCHIVE,
                 setup_exe.DirName().Append(kChromeCompressedArchive),
                 unpack_path, &uncompressed_archive) != UNPACK_NO_ERROR) {
        return base::unexpected(UNCOMPRESSION_FAILED);
      }
      if (uncompressed_archive.empty()) {
        LOG(ERROR) << "Failed to uncompress an archive from "
                   << setup_exe.DirName().Append(kChromeCompressedArchive);
        return base::unexpected(INVALID_ARCHIVE);
      }
      from_compressed_archive = true;
    }
  }

  if (uncompressed_archive.empty()) {
    LOG(ERROR) << "Cannot install Brave without an uncompressed archive.";
    return base::unexpected(INVALID_ARCHIVE);
  }

  // A compressed archive holds either chrome.7z or a patch file to be applied
  // to the installed version's chrome.7z.
  const base::FilePath target = unpack_path.Append(kChromeArchive);
  if (from_compressed_archive && !base::PathExists(target)) {
    RETURN_IF_ERROR(
        ApplyChromeArchivePatch(unpack_path, cmd_line, installer_state,
                                /*patch_file=*/uncompressed_archive, target));
    uncompressed_archive = target;
    installer_state.archive_type = INCREMENTAL_ARCHIVE_TYPE;
  } else {
    installer_state.archive_type = FULL_ARCHIVE_TYPE;
  }
  installer_state.uncompressed_archive = uncompressed_archive;

  installer_state.SetStage(UNPACKING);
  if (Unpack(UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE, uncompressed_archive,
             unpack_path, nullptr) != UNPACK_NO_ERROR) {
    return base::unexpected(UNPACKING_FAILED);
  }

  return base::ok();
}

}  // namespace

base::expected<void, InstallStatus> UnpackChromeArchive(
    const base::FilePath& unpack_path,
    const base::FilePath& setup_exe,
    const base::CommandLine& cmd_line,
    InstallerState& installer_state) {
  RETURN_IF_ERROR(BraveUnpackChromeArchiveImpl(unpack_path, setup_exe, cmd_line,
                                               installer_state),
                  [&installer_state](InstallStatus install_status) {
                    installer_state.WriteInstallerResult(
                        install_status,
                        install_status == INVALID_ARCHIVE
                            ? IDS_INSTALL_INVALID_ARCHIVE_BASE
                            : IDS_INSTALL_UNCOMPRESSION_FAILED_BASE,
                        nullptr);
                    return install_status;
                  });
  return base::ok();
}

}  // namespace installer
