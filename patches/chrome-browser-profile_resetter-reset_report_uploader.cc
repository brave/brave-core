diff --git a/chrome/browser/profile_resetter/reset_report_uploader.cc b/chrome/browser/profile_resetter/reset_report_uploader.cc
index 696cd4a6127f..24e6ce8873ed 100644
--- a/chrome/browser/profile_resetter/reset_report_uploader.cc
+++ b/chrome/browser/profile_resetter/reset_report_uploader.cc
@@ -48,6 +48,7 @@ void ResetReportUploader::DispatchReport(
 
 void ResetReportUploader::DispatchReportInternal(
     const std::string& request_data) {
+  return; // feature disabled in Brave
   // Create traffic annotation tag.
   net::NetworkTrafficAnnotationTag traffic_annotation =
       net::DefineNetworkTrafficAnnotation("profile_resetter_upload", R"(
