/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration_win.h"

#include <shlobj.h>
#include <wrl/client.h>

#include <memory>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/win/windows_version.h"
#include "chrome/browser/shell_integration_win.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

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

bool PinShortcutWin10(const base::FilePath& shortcut) {
  Microsoft::WRL::ComPtr<IPinnedList3> pinned_list = GetTaskbarPinnedList();
  if (!pinned_list)
    return false;

  ScopedPIDLFromPath item_id_list(shortcut.value().data());
  HRESULT hr = pinned_list->Modify(nullptr, item_id_list.Get(),
                                   PinnedListModifyCaller::kExplorer);
  return SUCCEEDED(hr);
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

void PinToTaskbarImpl() {
  base::FilePath chrome_exe;
  if (!base::PathService::Get(base::FILE_EXE, &chrome_exe))
    return;

  ShellUtil::ShortcutProperties properties(ShellUtil::CURRENT_USER);
  ShellUtil::AddDefaultShortcutProperties(chrome_exe, &properties);
  // Generate the shortcut path and ask to pin it.
  base::FilePath shortcut_path;
  ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_DESKTOP,
                             ShellUtil::CURRENT_USER, &shortcut_path);
  std::wstring shortcut_name(ExtractShortcutNameFromProperties(properties));
  shortcut_path = shortcut_path.Append(shortcut_name);

  bool create_shortcut = true;
  if (base::PathExists(shortcut_path))
    create_shortcut = false;

  if (create_shortcut && !ShellUtil::CreateOrUpdateShortcut(
                             ShellUtil::SHORTCUT_LOCATION_DESKTOP, properties,
                             ShellUtil::SHELL_SHORTCUT_CREATE_ALWAYS)) {
    LOG(ERROR) << __func__ << " Failed to create shortcut.";
    return;
  }

  auto pinned = IsShortcutPinnedWin10(shortcut_path);
  // Don't try to pin when checking is failed or it's already pinned.
  if (!pinned || *pinned)
    return;

  PinShortcutWin10(shortcut_path);
}

}  // namespace

namespace shell_integration::win {

// TODO(simonhong): Support profile specific shortcut pinning.
// For now, only default profile's shortcut is created and pinned.
void PinToTaskbar() {
  if (base::win::GetVersion() < base::win::Version::WIN10_RS5)
    return;

  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTask(FROM_HERE, base::BindOnce(&PinToTaskbarImpl));
}

}  // namespace shell_integration::win
