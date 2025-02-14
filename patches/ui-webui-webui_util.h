diff --git a/ui/webui/webui_util.h b/ui/webui/webui_util.h
index b599083ab67e4..73a1c4ba19eec 100644
--- a/ui/webui/webui_util.h
+++ b/ui/webui/webui_util.h
@@ -22,7 +22,7 @@ inline constexpr char kDefaultTrustedTypesPolicies[] =
     // Add TrustedTypes policies necessary for using Polymer.
     "polymer-html-literal polymer-template-event-attribute-policy "
     // Add TrustedTypes policies necessary for using Desktop's Lit bundle.
-    "lit-html-desktop";
+    "lit-html-desktop lit-mangler";
 
 // Performs common setup steps for a |source| using JS modules: enable i18n
 // string replacements, adding test resources, and configuring script-src CSP
