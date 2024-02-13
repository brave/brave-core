/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration_win.h"

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/win/shortcut.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_shortcut_manager_win.h"
#include "chrome/browser/shell_integration_win.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/taskbar_util.h"
#include "content/public/browser/browser_thread.h"

namespace {

class ScopedShortcutFile {
 public:
  explicit ScopedShortcutFile(const base::FilePath& file_path)
      : file_path_(file_path) {
    DCHECK(!file_path_.empty());
  }
  ScopedShortcutFile(const ScopedShortcutFile&) = delete;
  ScopedShortcutFile& operator=(const ScopedShortcutFile&) = delete;
  ~ScopedShortcutFile() { base::DeleteFile(file_path_); }

  const base::FilePath& file_path() const { return file_path_; }

 private:
  const base::FilePath file_path_;
};

std::wstring ExtractShortcutNameFromProperties(
    const ShellUtil::ShortcutProperties& properties) {
  std::wstring shortcut_name = properties.has_shortcut_name()
                                   ? properties.shortcut_name
                                   : InstallUtil::GetShortcutName();

  if (!base::EndsWith(shortcut_name, installer::kLnkExt,
                      base::CompareCase::INSENSITIVE_ASCII))
    shortcut_name += installer::kLnkExt;

  return shortcut_name;
}

bool CreateShortcut(const ShellUtil::ShortcutProperties& properties,
                    const base::FilePath& shortcut_path) {
  base::win::ShortcutProperties shortcut_properties;

  if (properties.has_target()) {
    shortcut_properties.set_target(properties.target);
    DCHECK(!properties.target.DirName().empty());
    shortcut_properties.set_working_dir(properties.target.DirName());
  }

  if (properties.has_arguments())
    shortcut_properties.set_arguments(properties.arguments);

  if (properties.has_description())
    shortcut_properties.set_description(properties.description);

  if (properties.has_icon())
    shortcut_properties.set_icon(properties.icon, properties.icon_index);

  if (properties.has_app_id())
    shortcut_properties.set_app_id(properties.app_id);

  if (properties.has_toast_activator_clsid()) {
    shortcut_properties.set_toast_activator_clsid(
        properties.toast_activator_clsid);
  }
  return base::win::CreateOrUpdateShortcutLink(
      shortcut_path, shortcut_properties,
      base::win::ShortcutOperation::kCreateAlways);
}

std::optional<ScopedShortcutFile> GetShortcutPath(
    const std::wstring& shortcut_name) {
  base::FilePath shortcut_path;
  if (!base::PathService::Get(base::DIR_TEMP, &shortcut_path) ||
      shortcut_path.empty())
    return std::nullopt;

  shortcut_path = shortcut_path.Append(shortcut_name);
  return std::optional<ScopedShortcutFile>(shortcut_path);
}

// All args could be empty when we want to pin default profile's shortcut.
bool PinToTaskbarImpl(const base::FilePath& profile_path,
                      const std::u16string& profile_name,
                      const std::wstring& aumid) {
  base::FilePath chrome_exe;
  if (!base::PathService::Get(base::FILE_EXE, &chrome_exe))
    return false;

  ShellUtil::ShortcutProperties properties(ShellUtil::CURRENT_USER);
  ShellUtil::AddDefaultShortcutProperties(chrome_exe, &properties);

  const bool pin_profile_specific_shortcut = !profile_path.empty();
  if (pin_profile_specific_shortcut) {
    properties.set_arguments(
        profiles::internal::CreateProfileShortcutFlags(profile_path));
    properties.set_shortcut_name(
        profiles::internal::GetShortcutFilenameForProfile(profile_name));
    properties.set_app_id(aumid);

    const base::FilePath icon_path =
        profiles::internal::GetProfileIconPath(profile_path);
    if (base::PathExists(icon_path))
      properties.set_icon(icon_path, 0);
  }

  // Generate shortcut file to use for pin to taskbar.
  std::optional<ScopedShortcutFile> shortcut_path =
      GetShortcutPath(ExtractShortcutNameFromProperties(properties));
  if (!shortcut_path) {
    LOG(ERROR) << __func__ << " failed to get shortcut path";
    return false;
  }

  if (!CreateShortcut(properties, shortcut_path->file_path())) {
    LOG(ERROR) << __func__ << " Failed to create shortcut";
    return false;
  }

  return PinShortcutToTaskbar(shortcut_path->file_path());
}

void DoPinToTaskbar(const base::FilePath& profile_path,
                    base::OnceCallback<void(bool)> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::u16string profile_name;
  std::wstring aumid;
  if (!profile_path.empty()) {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ProfileAttributesEntry* entry =
        profile_manager->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile_path);
    profile_name = entry->GetName();
    aumid = shell_integration::win::GetAppUserModelIdForBrowser(profile_path);
  }

  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&PinToTaskbarImpl, profile_path, profile_name, aumid),
          std::move(callback));
}

}  // namespace

namespace shell_integration::win {

void PinToTaskbar(Profile* profile,
                  base::OnceCallback<void(bool)> result_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!CanPinShortcutToTaskbar()) {
    std::move(result_callback).Run(false);
    return;
  }

  base::FilePath profile_path;
  if (profile)
    profile_path = profile->GetPath();

  GetIsPinnedToTaskbarState(base::BindOnce(
      [](const base::FilePath& profile_path,
         base::OnceCallback<void(bool)> result_callback, bool succeeded,
         bool is_pinned_to_taskbar) {
        if (succeeded && is_pinned_to_taskbar) {
          // Early return. Already pinned.
          std::move(result_callback).Run(true);
          return;
        }
        DoPinToTaskbar(profile_path, std::move(result_callback));
      },
      std::move(profile_path), std::move(result_callback)));
}

void IsShortcutPinned(base::OnceCallback<void(bool)> result_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!CanPinShortcutToTaskbar()) {
    std::move(result_callback).Run(false);
    return;
  }

  GetIsPinnedToTaskbarState(base::BindOnce(
      [](base::OnceCallback<void(bool)> result_callback, bool succeeded,
         bool is_pinned_to_taskbar) {
        std::move(result_callback).Run(succeeded && is_pinned_to_taskbar);
      },
      std::move(result_callback)));
}

}  // namespace shell_integration::win
