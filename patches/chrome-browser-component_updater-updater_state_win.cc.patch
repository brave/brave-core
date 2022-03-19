diff --git a/chrome/browser/component_updater/updater_state_win.cc b/chrome/browser/component_updater/updater_state_win.cc
index 8030caa0f8ca5a9e77fd982c4b01fac4b35cc9b3..72f36ff6fbb919d048256bdbe592bbad8093334b 100644
--- a/chrome/browser/component_updater/updater_state_win.cc
+++ b/chrome/browser/component_updater/updater_state_win.cc
@@ -24,20 +24,32 @@ namespace {
 
 // Google Update group policy settings.
 const wchar_t kGoogleUpdatePoliciesKey[] =
+    L"SOFTWARE\\Policies\\BraveSoftware\\Update";
+#if 0
     L"SOFTWARE\\Policies\\Google\\Update";
+#endif
 const wchar_t kCheckPeriodOverrideMinutes[] = L"AutoUpdateCheckPeriodMinutes";
 const wchar_t kUpdatePolicyValue[] = L"UpdateDefault";
 const wchar_t kChromeUpdatePolicyOverride[] =
+    L"Update{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
+#if 0
     L"Update{8A69D345-D564-463C-AFF1-A69D9E530F96}";
+#endif
 
 // Don't allow update periods longer than six weeks (Chrome release cadence).
 const int kCheckPeriodOverrideMinutesMax = 60 * 24 * 7 * 6;
 
 // Google Update registry settings.
+const wchar_t kRegPathGoogleUpdate[] = L"Software\\BraveSoftware\\Update";
+const wchar_t kRegPathClientsGoogleUpdate[] =
+    L"Software\\BraveSoftware\\Update\\Clients\\"
+    L"{B131C935-9BE6-41DA-9599-1F776BEB8019}";
+#if 0
 const wchar_t kRegPathGoogleUpdate[] = L"Software\\Google\\Update";
 const wchar_t kRegPathClientsGoogleUpdate[] =
     L"Software\\Google\\Update\\Clients\\"
     L"{430FD4D0-B729-4F61-AA34-91526481799D}";
+#endif
 const wchar_t kRegValueGoogleUpdatePv[] = L"pv";
 const wchar_t kRegValueLastStartedAU[] = L"LastStartedAU";
 const wchar_t kRegValueLastChecked[] = L"LastChecked";
