/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_donate_ui.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

BraveDonateUI::BraveDonateUI(content::WebUI* web_ui, const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  content::WebUIDataSource* data_source = CreateBasicUIHTMLSource(profile,
                                              name,
                                              kDonateJS,
                                              IDR_BRAVE_DONATE_JS,
                                              IDR_BRAVE_DONATE_HTML);
  content::WebUIDataSource::Add(profile, data_source);
}

BraveDonateUI::~BraveDonateUI() {
}
