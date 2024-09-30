#include <stddef.h>

namespace chrome {
inline constexpr char kPrivacySandboxManageTopicsURL[] =
    "chrome://settings/adPrivacy/interests/manage";
inline constexpr char kPrivacySandboxAdTopicsURL[] =
    "chrome://settings/adPrivacy/interests";
}

#define StartWithTwoBlockedTopics DISABLED_StartWithTwoBlockedTopics
#define BlockFirstTopicOnManageTopicsPage DISABLED_BlockFirstTopicOnManageTopicsPage
#define UnblockOneTopicOnAdTopicsPage DISABLED_UnblockOneTopicOnAdTopicsPage
#define ConfirmDefaultIconIsNotUsed DISABLED_ConfirmDefaultIconIsNotUsed
#include "src/chrome/browser/privacy_sandbox/browsing_topics_settings_interactive_uitest.cc"
#undef StartWithTwoBlockedTopics
#undef BlockFirstTopicOnManageTopicsPage
#undef UnblockOneTopicOnAdTopicsPage
#undef ConfirmDefaultIconIsNotUsed