#include "chrome/browser/permissions/permission_request_impl.h"
#define GetIconId GetIconId_ChromiumImpl
#define GetMessageTextFragment GetMessageTextFragment_ChromiumImpl
#include "../../../../../chrome/browser/permissions/permission_request_impl.cc"
#undef GetMessageTextFragment
#undef GetIconId

PermissionRequest::IconId PermissionRequestImpl::GetIconId() const {
#if !defined(OS_ANDROID)
  switch (content_settings_type_) {
    case CONTENT_SETTINGS_TYPE_AUTOPLAY:
      return vector_icons::kPlayArrowIcon;
    default:
      break;
  }
#endif
  return GetIconId_ChromiumImpl();
}

base::string16 PermissionRequestImpl::GetMessageTextFragment() const {
  int message_id;
  switch (content_settings_type_) {
    case CONTENT_SETTINGS_TYPE_AUTOPLAY:
      message_id = IDS_AUTOPLAY_PERMISSION_FRAGMENT;
      break;
    default:
      return GetMessageTextFragment_ChromiumImpl();
  }
  return l10n_util::GetStringUTF16(message_id);
}
