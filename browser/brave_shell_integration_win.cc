/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration_win.h"

#include <shlobj.h>
#include <wrl/client.h>

#include <memory>
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
#include "base/win/windows_version.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_shortcut_manager_win.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/shell_integration_win.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

absl::optional<ScopedShortcutFile> GetShortcutPath(
    const std::wstring& shortcut_name) {
  base::FilePath shortcut_path;
  if (!base::PathService::Get(base::DIR_TEMP, &shortcut_path) ||
      shortcut_path.empty())
    return absl::nullopt;

  shortcut_path = shortcut_path.Append(shortcut_name);
  return absl::optional<ScopedShortcutFile>(shortcut_path);
}

// NOTE: Below Pin/IsPin method is copied lastest chromium.
// Delete and use upstreams one when it's available from our trunk.

// ScopedPIDLFromPath class, and the idea of using IPinnedList3::Modify,
// are thanks to Gee Law <https://geelaw.blog/entries/msedge-pins/>
class ScopedPIDLFromPath {
 public:
  explicit ScopedPIDLFromPath(PCWSTR path)
      : p_id_list_(ILCreateFromPath(path)) {}
  ~ScopedPIDLFromPath() {
    if (p_id_list_)
      ILFree(p_id_list_);
  }
  PIDLIST_ABSOLUTE Get() const { return p_id_list_; }

 private:
  PIDLIST_ABSOLUTE const p_id_list_;
};

enum class PinnedListModifyCaller { kExplorer = 4 };

constexpr GUID CLSID_TaskbandPin = {
    0x90aa3a4e,
    0x1cba,
    0x4233,
    {0xb8, 0xbb, 0x53, 0x57, 0x73, 0xd4, 0x84, 0x49}};

// Undocumented COM interface for manipulating taskbar pinned list.
class __declspec(uuid("0DD79AE2-D156-45D4-9EEB-3B549769E940")) IPinnedList3
    : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE EnumObjects() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetPinnableInfo() = 0;
  virtual HRESULT STDMETHODCALLTYPE IsPinnable() = 0;
  virtual HRESULT STDMETHODCALLTYPE Resolve() = 0;
  virtual HRESULT STDMETHODCALLTYPE LegacyModify() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetChangeCount() = 0;
  virtual HRESULT STDMETHODCALLTYPE IsPinned(PCIDLIST_ABSOLUTE) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetPinnedItem() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetAppIDForPinnedItem() = 0;
  virtual HRESULT STDMETHODCALLTYPE ItemChangeNotify() = 0;
  virtual HRESULT STDMETHODCALLTYPE UpdateForRemovedItemsAsNecessary() = 0;
  virtual HRESULT STDMETHODCALLTYPE PinShellLink() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetPinnedItemForAppID() = 0;
  virtual HRESULT STDMETHODCALLTYPE Modify(PCIDLIST_ABSOLUTE unpin,
                                           PCIDLIST_ABSOLUTE pin,
                                           PinnedListModifyCaller caller) = 0;
};

// Returns the taskbar pinned list if successful, an empty ComPtr otherwise.
Microsoft::WRL::ComPtr<IPinnedList3> GetTaskbarPinnedList() {
  if (base::win::GetVersion() < base::win::Version::WIN10_RS5)
    return nullptr;

  Microsoft::WRL::ComPtr<IPinnedList3> pinned_list;
  if (FAILED(CoCreateInstance(CLSID_TaskbandPin, nullptr, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&pinned_list)))) {
    return nullptr;
  }

  return pinned_list;
}

void PinShortcutWin10(const base::FilePath& shortcut) {
  Microsoft::WRL::ComPtr<IPinnedList3> pinned_list = GetTaskbarPinnedList();
  if (!pinned_list)
    return;

  ScopedPIDLFromPath item_id_list(shortcut.value().data());
  pinned_list->Modify(nullptr, item_id_list.Get(),
                      PinnedListModifyCaller::kExplorer);
}

absl::optional<bool> IsShortcutPinnedWin10(const base::FilePath& shortcut) {
  Microsoft::WRL::ComPtr<IPinnedList3> pinned_list = GetTaskbarPinnedList();
  if (!pinned_list.Get())
    return absl::nullopt;

  ScopedPIDLFromPath item_id_list(shortcut.value().data());
  HRESULT hr = pinned_list->IsPinned(item_id_list.Get());
  // S_OK means `shortcut` is pinned, S_FALSE mean it's not pinned.
  return SUCCEEDED(hr) ? absl::optional<bool>(hr == S_OK) : absl::nullopt;
}

bool IsShortcutPinned(const ShellUtil::ShortcutProperties& properties) {
  // Generate the shortcut to check pin state.
  absl::optional<ScopedShortcutFile> shortcut_path =
      GetShortcutPath(ExtractShortcutNameFromProperties(properties));
  if (!shortcut_path) {
    LOG(ERROR) << __func__ << " failed to get shortcut path";
    return false;
  }

  if (!CreateShortcut(properties, shortcut_path->file_path())) {
    LOG(ERROR) << __func__ << " Failed to create shortcut";
    return false;
  }

  // Check pin state with newly created shortcut.
  auto pinned = IsShortcutPinnedWin10(shortcut_path->file_path());
  if (!pinned) {
    LOG(ERROR) << __func__ << " Can't use pin method.";
    return false;
  }

  return pinned.value();
}

// All args could be empty when we want to pin default profile's shortcut.
void PinToTaskbarImpl(const base::FilePath& profile_path,
                      const std::u16string& profile_name,
                      const std::wstring& aumid) {
  base::FilePath chrome_exe;
  base::PathService::Get(base::FILE_EXE, &chrome_exe);

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
  absl::optional<ScopedShortcutFile> shortcut_path =
      GetShortcutPath(ExtractShortcutNameFromProperties(properties));
  if (!shortcut_path) {
    LOG(ERROR) << __func__ << " failed to get shortcut path";
    return;
  }

  if (!CreateShortcut(properties, shortcut_path->file_path())) {
    LOG(ERROR) << __func__ << " Failed to create shortcut";
    return;
  }

  // Check pin state with newly created shortcut.
  auto pinned = IsShortcutPinnedWin10(shortcut_path->file_path());
  if (!pinned) {
    LOG(ERROR) << __func__ << " Can't use pin method.";
    return;
  }

  // Don't try to pin again when it's already pinned.
  if (pinned.value())
    return;

  PinShortcutWin10(shortcut_path->file_path());
}

bool HasTaskbarAnyPinnedBraveShortcuts(
    const std::vector<std::tuple<base::FilePath, std::u16string, std::wstring>>&
        profile_attrs) {
  base::FilePath chrome_exe;
  base::PathService::Get(base::FILE_EXE, &chrome_exe);

  ShellUtil::ShortcutProperties properties(ShellUtil::CURRENT_USER);
  ShellUtil::AddDefaultShortcutProperties(chrome_exe, &properties);
  for (const auto& attr : profile_attrs) {
    const auto profile_path = std::get<0>(attr);
    const auto profile_name = std::get<1>(attr);
    const auto profile_aumid = std::get<2>(attr);
    if (!profile_path.empty()) {
      properties.set_arguments(
          profiles::internal::CreateProfileShortcutFlags(profile_path));
      properties.set_shortcut_name(
          profiles::internal::GetShortcutFilenameForProfile(profile_name));
      properties.set_app_id(profile_aumid);
    }

    if (IsShortcutPinned(properties))
      return true;
  }

  return false;
}

void DoPinToTaskbar(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::FilePath profile_path;
  std::u16string profile_name;
  std::wstring aumid;
  if (profile) {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ProfileAttributesEntry* entry =
        profile_manager->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile->GetPath());
    profile_path = profile->GetPath();
    profile_name = entry->GetName();
    aumid = shell_integration::win::GetAppUserModelIdForBrowser(profile_path);
  }

  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTask(FROM_HERE, base::BindOnce(&PinToTaskbarImpl, profile_path,
                                           profile_name, aumid));
}

bool CanPinToTaskbar() {
  base::FilePath chrome_exe;
  if (!base::PathService::Get(base::FILE_EXE, &chrome_exe))
    return false;

  // TODO(simonhong): Support win7/8
  if (base::win::GetVersion() < base::win::Version::WIN10_RS5)
    return false;

  return true;
}

}  // namespace

namespace shell_integration::win {

void PinToTaskbar(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!CanPinToTaskbar())
    return;

  // At the very early stage, |g_browser_process| or its profile_manager
  // are not initialzied yet. In that case, skip checking existing pin state.
  std::vector<std::tuple<base::FilePath, std::u16string, std::wstring>>
      profile_attrs;
  // Gather data that is available on UI thread and pass it.
  if (g_browser_process && g_browser_process->profile_manager()) {
    for (const auto* entry : g_browser_process->profile_manager()
                                 ->GetProfileAttributesStorage()
                                 .GetAllProfilesAttributes()) {
      profile_attrs.push_back(
          std::make_tuple(entry->GetPath(), entry->GetName(),
                          win::GetAppUserModelIdForBrowser(entry->GetPath())));
    }
  }

  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&HasTaskbarAnyPinnedBraveShortcuts,
                         std::move(profile_attrs)),
          base::BindOnce(
              [](Profile* profile, bool has_pin) {
                if (has_pin) {
                  VLOG(2) << " Taskbar has already pinned brave shortcuts";
                  return;
                }
                DoPinToTaskbar(profile);
              },
              profile));
  return;
}

void PinDefaultShortcutForExistingUsers() {
  // TODO(simonhong): Support win7/8
  // Below win::PinToTaskbar() doesn't work for win7/8.
  if (base::win::GetVersion() < base::win::Version::WIN10_RS5)
    return;

  if (first_run::IsChromeFirstRun())
    return;

  auto* local_prefs = g_browser_process->local_state();
  if (!local_prefs->GetBoolean(kTryToPinForExistingUsers))
    return;

  // Do this only once.
  local_prefs->SetBoolean(kTryToPinForExistingUsers, false);

  // Try to pin default shortcut when existing user already set Brave as a
  // default browser.
  auto set_browser_worker = base::MakeRefCounted<DefaultBrowserWorker>();
  set_browser_worker->StartCheckIsDefault(
      base::BindOnce([](shell_integration::DefaultWebClientState state) {
        if (state == shell_integration::IS_DEFAULT) {
          win::PinToTaskbar();
        }
      }));
}

}  // namespace shell_integration::win
