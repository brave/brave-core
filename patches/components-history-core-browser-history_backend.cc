--- a/components/history/core/browser/history_backend.cc	2022-06-12 05:25:32.361826400 +0700
+++ b/components/history/core/browser/history_backend.cc	2022-06-12 05:27:20.721826000 +0700
@@ -1008,7 +1008,9 @@
   db_->GetStartDate(&first_recorded_time_);
 
   // Start expiring old stuff.
-  expirer_.StartExpiringOldStuff(base::Days(kExpireDaysThreshold));
+  if (!KeepOldHistory()) {
+    expirer_.StartExpiringOldStuff(base::Days(kExpireDaysThreshold));
+  }
 
   LOCAL_HISTOGRAM_TIMES("History.InitTime", TimeTicks::Now() - beginning_time);
 }
