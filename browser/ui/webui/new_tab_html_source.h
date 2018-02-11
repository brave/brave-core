/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

namespace content {
class WebUIDataSource;
class RenderViewHost;
class WebUI;
}

class Profile;

void CustomizeNewTabHTMLSource(Profile* profile, content::WebUIDataSource* source);
void CustomizeNewTabWebUIProperties(content::WebUI* web_ui, Profile* profile, content::RenderViewHost* render_view_host);
