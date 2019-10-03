/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"

#include <map>
#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/tor/tor_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace brave {

namespace {

using PathMap = std::map<const base::FilePath, Profile*>;

PathMap* GetPathMap() {
  static base::NoDestructor<PathMap> provider;
  return provider.get();
}

Profile* GetFromPath(const base::FilePath& key) {
  auto mapping = *GetPathMap();
  const auto& it = mapping.find(key);
  if (it == mapping.end()) {
    DCHECK(false);
    return nullptr;
  }

  return it->second;
}

class ParentProfileData : public base::SupportsUserData::Data {
 public:
  ~ParentProfileData() override;
  static void CreateForProfile(content::BrowserContext* context);
  static ParentProfileData* FromProfile(content::BrowserContext* context);
  static const ParentProfileData* FromProfile(
      const content::BrowserContext* context);
  static const ParentProfileData* FromPath(const base::FilePath& path);

  Profile* profile() const;

  std::unique_ptr<Data> Clone() override;

 private:
  static const void* const kUserDataKey;
  static const void* UserDataKey();

  explicit ParentProfileData(Profile* profile);

  Profile* profile_;
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(ParentProfileData);
};

const void* const ParentProfileData::kUserDataKey = &kUserDataKey;

// static
void ParentProfileData::CreateForProfile(content::BrowserContext* context) {
  DCHECK(context);
  if (FromProfile(context))
    return;

  auto* profile = Profile::FromBrowserContext(context);
  auto* profile_manager = g_browser_process->profile_manager();
  DCHECK(profile_manager);

  auto* parent_profile =
      profile_manager->GetProfileByPath(GetParentProfilePath(profile));
  DCHECK(parent_profile);
  DCHECK(parent_profile != profile);

  profile->SetUserData(UserDataKey(),
                       base::WrapUnique(new ParentProfileData(parent_profile)));

  GetPathMap()->insert(
      std::pair<const base::FilePath, Profile*>(profile->GetPath(), profile));
}

// static
ParentProfileData* ParentProfileData::FromProfile(
    content::BrowserContext* context) {
  DCHECK(context);
  auto* profile = Profile::FromBrowserContext(context);
  return static_cast<ParentProfileData*>(
      profile->GetOriginalProfile()->GetUserData(UserDataKey()));
}

// static
const ParentProfileData* ParentProfileData::FromProfile(
    const content::BrowserContext* context) {
  DCHECK(context);
  const auto* profile = static_cast<const Profile*>(context);
  return static_cast<const ParentProfileData*>(
      profile->GetOriginalProfile()->GetUserData(UserDataKey()));
}

// static
const ParentProfileData* ParentProfileData::FromPath(
    const base::FilePath& path) {
  auto* profile = GetFromPath(path);
  DCHECK(profile);
  return FromProfile(profile);
}

Profile* ParentProfileData::profile() const {
  return profile_;
}

std::unique_ptr<base::SupportsUserData::Data> ParentProfileData::Clone() {
  return base::WrapUnique(new ParentProfileData(profile_));
}

const void* ParentProfileData::UserDataKey() {
  return &kUserDataKey;
}

ParentProfileData::ParentProfileData(Profile* profile)
    : profile_(profile), path_(profile->GetPath()) {}

ParentProfileData::~ParentProfileData() {
  GetPathMap()->erase(path_);
}

}  // namespace

Profile* CreateParentProfileData(content::BrowserContext* context) {
  ParentProfileData::CreateForProfile(context);
  return ParentProfileData::FromProfile(context)->profile();
}

base::FilePath GetParentProfilePath(content::BrowserContext* context) {
  return GetParentProfilePath(context->GetPath());
}

base::FilePath GetParentProfilePath(const base::FilePath& path) {
  return path.DirName().DirName();
}

bool IsSessionProfile(content::BrowserContext* context) {
  DCHECK(context);
  return ParentProfileData::FromProfile(context) != nullptr;
}

bool IsSessionProfilePath(const base::FilePath& path) {
  return path.DirName().BaseName() ==
         base::FilePath(FILE_PATH_LITERAL("session_profiles"));
}

bool IsTorProfilePath(const base::FilePath& path) {
#if BUILDFLAG(ENABLE_TOR)
  return IsSessionProfilePath(path) &&
         path.BaseName() == base::FilePath(tor::kTorProfileDir);
#else
  return false;
#endif
}

bool IsTorProfile(content::BrowserContext* context) {
#if BUILDFLAG(ENABLE_TOR)
  DCHECK(context);
  return IsTorProfilePath(
      Profile::FromBrowserContext(context)->GetOriginalProfile()->GetPath());
#else
  return false;
#endif
}

Profile* GetParentProfile(content::BrowserContext* context) {
  DCHECK(context);
  return ParentProfileData::FromProfile(context)->profile();
}

Profile* GetParentProfile(const base::FilePath& path) {
  return ParentProfileData::FromPath(path)->profile();
}

bool IsGuestProfile(content::BrowserContext* context) {
  DCHECK(context);
  return Profile::FromBrowserContext(context)
      ->GetOriginalProfile()
      ->IsGuestSession();
}

}  // namespace brave

namespace chrome {

// Get the correct profile for keyed services that use
// GetBrowserContextRedirectedInIncognito or equivalent
content::BrowserContext* GetBrowserContextRedirectedInIncognitoOverride(
    content::BrowserContext* context) {
  if (brave::IsSessionProfile(context))
    context = brave::GetParentProfile(context);
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace chrome
