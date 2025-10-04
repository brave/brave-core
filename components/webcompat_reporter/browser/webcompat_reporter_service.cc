// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"
#include "brave/components/webcompat_reporter/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

struct ReportFiller {
  ReportFiller(
      webcompat_reporter::mojom::ReportInfoPtr& report_info,
      webcompat_reporter::WebcompatReporterService::Delegate* service_delegate)
      : report_info(report_info), service_delegate(service_delegate) {}
  ~ReportFiller() = default;

  ReportFiller& FillReportWithAdblockListNames() {
    if ((*report_info)->ad_block_list_names &&
        !(*report_info)->ad_block_list_names->empty()) {
      return *this;
    }

    if (!service_delegate) {
      return *this;
    }

    auto filter_list = service_delegate->GetAdblockFilterListNames();
    if (!filter_list) {
      return *this;
    }
    (*report_info)->ad_block_list_names =
        base::JoinString(filter_list.value(), ",");
    return *this;
  }

  ReportFiller& FillReportWithComponetsInfo() {
    if ((*report_info)->ad_block_components_version &&
        !(*report_info)->ad_block_components_version->empty()) {
      return *this;
    }
    if (!service_delegate) {
      return *this;
    }

    auto all_components = service_delegate->GetComponentInfos();
    std::vector<
        webcompat_reporter::WebcompatReporterService::Delegate::ComponentInfo>
        components_to_send = {};
    std::copy_if(
        all_components.begin(), all_components.end(),
        std::back_inserter(components_to_send),
        [](webcompat_reporter::WebcompatReporterService::Delegate::ComponentInfo
               ci) {
          return webcompat_reporter::SendComponentVersionInReport(ci.id);
        });
    std::vector<webcompat_reporter::mojom::ComponentInfoPtr>
        mojom_components_to_send = {};
    std::transform(
        components_to_send.begin(), components_to_send.end(),
        std::back_inserter(mojom_components_to_send),
        [](webcompat_reporter::WebcompatReporterService::Delegate::ComponentInfo
               ci) {
          return webcompat_reporter::mojom::ComponentInfo::New(ci.name, ci.id,
                                                               ci.version);
        });

    if (components_to_send.empty()) {
      return *this;
    }

    (*report_info)->ad_block_components_version =
        std::move(mojom_components_to_send);
    return *this;
  }

  ReportFiller& FillChannel() {
    if (!(*report_info)->channel && service_delegate) {
      (*report_info)->channel = service_delegate->GetChannelName();
    }
    return *this;
  }

  ReportFiller& FillVersion() {
    if (!(*report_info)->brave_version) {
      (*report_info)->brave_version =
          version_info::GetBraveVersionWithoutChromiumMajorVersion();
    }
    return *this;
  }

  ReportFiller& FillCookiePolicy() {
    if (!(*report_info)->cookie_policy) {
      (*report_info)->cookie_policy =
          service_delegate->GetCookiePolicy((*report_info)->report_url);
    }
    return *this;
  }

  ReportFiller& FillScriptBlockingFlag() {
    if (!(*report_info)->block_scripts) {
      (*report_info)->block_scripts =
          service_delegate->GetScriptBlockingFlag((*report_info)->report_url);
    }
    return *this;
  }

  raw_ref<webcompat_reporter::mojom::ReportInfoPtr> report_info;
  const raw_ptr<webcompat_reporter::WebcompatReporterService::Delegate>
      service_delegate;
};

void ProcessContactInfo(
    PrefService* profile_prefs,
    const webcompat_reporter::mojom::ReportInfoPtr& report_info) {
  if (!profile_prefs) {
    return;
  }
  if (!report_info->contact || report_info->contact->empty()) {
    profile_prefs->ClearPref(webcompat_reporter::prefs::kContactInfoPrefs);
    return;
  }

  profile_prefs->SetString(
      webcompat_reporter::prefs::kContactInfoPrefs,
      profile_prefs->GetBooleanOr(
          webcompat_reporter::prefs::kContactInfoSaveFlagPrefs, false)
          ? report_info->contact.value()
          : "");
}

std::vector<std::string> GetInstalledComponentIds(
    webcompat_reporter::WebcompatReporterService::Delegate const*
        service_delegate) {
  std::vector<std::string> component_ids;
  if (!service_delegate) {
    return component_ids;
  }

  const auto components = service_delegate->GetComponentInfos();
  component_ids.resize(components.size());

  std::transform(components.begin(), components.end(), component_ids.begin(),
                 [](auto c) { return c.id; });

  return component_ids;
}

using WebcompatCategory = webcompat_reporter::mojom::WebcompatCategory;

constexpr char kHideCookieNoticeCategoryForComponentId[] =
    "cdbbhgbmjhfnhnmgeddbliobbofkgdhe";
constexpr char kHideNewsletterCategoryForComponentId[] =
    "kdddfellohomdnfkdhombbddhojklibj";
constexpr char kHideSocialCategoryForComponentId[] =
    "nbkknaieglghmocpollinelcggiehfco";
constexpr char kHideChatCategoryForComponentId[] =
    "cjoooeeofnfjohnalnghhmdlalopplja";

bool HideIssueCategory(const std::vector<std::string>& component_ids,
                       const WebcompatCategory category) {
  if (category == WebcompatCategory::kCookieNotice &&
      !base::Contains(component_ids, kHideCookieNoticeCategoryForComponentId)) {
    return true;
  }

  if (category == WebcompatCategory::kNewsletter &&
      !base::Contains(component_ids, kHideNewsletterCategoryForComponentId)) {
    return true;
  }

  if (category == WebcompatCategory::kSocial &&
      !base::Contains(component_ids, kHideSocialCategoryForComponentId)) {
    return true;
  }

  if (category == WebcompatCategory::kChat &&
      !base::Contains(component_ids, kHideChatCategoryForComponentId)) {
    return true;
  }

  return false;
}

}  // namespace

namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService(
    PrefService* profile_prefs,
    std::unique_ptr<Delegate> service_delegate,
    std::unique_ptr<WebcompatReportUploader> report_uploader)
    : profile_prefs_(profile_prefs),
      service_delegate_(std::move(service_delegate)),
      report_uploader_(std::move(report_uploader)) {}

WebcompatReporterService::~WebcompatReporterService() = default;

mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterService::MakeRemote() {
  mojo::PendingRemote<mojom::WebcompatReporterHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void WebcompatReporterService::Bind(
    mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void WebcompatReporterService::SubmitWebcompatReport(
    mojom::ReportInfoPtr report_info) {
  ReportFiller(report_info,
               service_delegate_ ? service_delegate_.get() : nullptr)
      .FillChannel()
      .FillVersion()
      .FillReportWithComponetsInfo()
      .FillReportWithAdblockListNames()
      .FillCookiePolicy()
      .FillScriptBlockingFlag();

  ProcessContactInfo(profile_prefs_, report_info);

  report_uploader_->SubmitReport(std::move(report_info));
}

void WebcompatReporterService::SetContactInfoSaveFlag(bool value) {
  if (!profile_prefs_) {
    return;
  }
  if (!value) {
    profile_prefs_->SetString(prefs::kContactInfoPrefs, "");
  }
  profile_prefs_->SetBoolean(prefs::kContactInfoSaveFlagPrefs, value);
}

void WebcompatReporterService::GetBrowserParams(
    GetBrowserParamsCallback callback) {
  std::vector<std::string> component_ids(GetInstalledComponentIds(
      service_delegate_ ? service_delegate_.get() : nullptr));

  if (!profile_prefs_) {
    std::move(callback).Run(std::nullopt, false, std::move(component_ids));
    return;
  }
  auto save_flag_value =
      profile_prefs_->GetBoolean(prefs::kContactInfoSaveFlagPrefs);
  if (!save_flag_value) {
    std::move(callback).Run(std::nullopt, save_flag_value,
                            std::move(component_ids));
    return;
  }

  auto contact_value = profile_prefs_->GetString(prefs::kContactInfoPrefs);
  std::move(callback).Run(std::move(contact_value), save_flag_value,
                          std::move(component_ids));
}

void WebcompatReporterService::SetPrefServiceTest(PrefService* pref_service) {
  profile_prefs_ = pref_service;
}

void WebcompatReporterService::GetWebcompatCategories(
    GetWebcompatCategoriesCallback callback) {
  std::vector<std::string> component_ids(GetInstalledComponentIds(
      service_delegate_ ? service_delegate_.get() : nullptr));

  std::vector<mojom::WebcompatCategoryItemPtr> categories;
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kAds,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_ADS)),
      "ads"));
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kBrowserNotSupported,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_BROWSER_NOT_SUPPORTED)),
      "browser not supported"));
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kBlank,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_BLANK)),
      "blank"));
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kScroll,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_SCROLL)),
      "scroll"));
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kForm,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_FORM)),
      "form"));

  if (!HideIssueCategory(component_ids, WebcompatCategory::kCookieNotice)) {
    categories.emplace_back(mojom::WebcompatCategoryItem::New(
        WebcompatCategory::kCookieNotice,
        base::UTF16ToUTF8(l10n_util::GetStringUTF16(
            IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_COOKIE)),
        "cookie notice"));
  }

  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kAntiAdblock,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_ANTIADBLOCK)),
      "anti adblock"));
  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kTracking,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_TRACKING)),
      "tracking"));

  if (!HideIssueCategory(component_ids, WebcompatCategory::kNewsletter)) {
    categories.emplace_back(mojom::WebcompatCategoryItem::New(
        WebcompatCategory::kNewsletter,
        base::UTF16ToUTF8(l10n_util::GetStringUTF16(
            IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_NEWSLETTER)),
        "newsletter"));
  }

  if (!HideIssueCategory(component_ids, WebcompatCategory::kSocial)) {
    categories.emplace_back(mojom::WebcompatCategoryItem::New(
        WebcompatCategory::kSocial,
        base::UTF16ToUTF8(l10n_util::GetStringUTF16(
            IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_SOCIAL)),
        "social"));
  }

  if (!HideIssueCategory(component_ids, WebcompatCategory::kChat)) {
    categories.emplace_back(mojom::WebcompatCategoryItem::New(
        WebcompatCategory::kChat,
        base::UTF16ToUTF8(l10n_util::GetStringUTF16(
            IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_CHAT)),
        "chat"));
  }

  categories.emplace_back(mojom::WebcompatCategoryItem::New(
      WebcompatCategory::kOther,
      base::UTF16ToUTF8(l10n_util::GetStringUTF16(
          IDS_BRAVE_WEBCOMPATREPORTER_ISSUE_CATEGORY_OTHER)),
      "other"));

  std::move(callback).Run(std::move(categories));
}

}  // namespace webcompat_reporter
