#include "brave/browser/sharing_hub/brave_sharing_hub_model.h"

#include "brave/app/brave_command_ids.h"
#include "chrome/browser/sharing_hub/sharing_hub_model.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "components/strings/grit/components_strings.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "components/vector_icons/vector_icons.h"
#include "brave/components/l10n/common/locale_util.h"


namespace sharing_hub {

BraveSharingHubModel::BraveSharingHubModel(content::BrowserContext* context)
    : SharingHubModel(context) {
  // TODO: Proper share tooltip.
  int tooltip_id = IDS_SHARE_MENU_TITLE;

  first_party_action_list_.insert(first_party_action_list_.begin(),
      {IDC_BRAVE_TALK_SHARE_TAB,
       brave_l10n::GetLocalizedResourceUTF16String(tooltip_id), &vector_icons::kScreenShareIcon,
       true, gfx::ImageSkia(), "BraveTalk.ShareTab"});
}

BraveSharingHubModel::~BraveSharingHubModel() = default;

}  // namespace sharing_hub