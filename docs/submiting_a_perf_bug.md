# How to create a perf bug
Performance bugs are sophisticated and usually related to specific user profile state/software/hardware configuration. Frequently, issues are caused by drivers/ extensions/ third-party software. Therefore, we need more details to deal with it.

1. Can you reproduce the bug in Google Chrome or Chromium? If yes, please submit the bug directly to Chromium: https://bugs.chromium.org/p/chromium/issues/entry

2. Can you reproduce the problem in an incognito window / fresh profile / clean installation or on another device (if you have one)?

3. Does any of the following actions help to fix the issue?
 * disabling Shield;
 * disabling all extensions (`brave://extensions` page);
 * disabling hardware acceleration (`Settings` &rarr; `Additional settings` &rarr; `System` &rarr; `Use hardware acceleration when available`);
 * defaulting all experimental flags on `brave://flags` (if you modified them).

4. Write down the exact steps that lead to the problem (what we should do to see the same problem). A screencast will also help.

5. Save these pages (using `Ctrl + S`) and attach to the issue.
* `brave://version`;
* `brave://histograms`;
* `brave://system/`;
* `brave://crashes` (could be crash dumps related to out-of memory);
* `brave://gpu` in case of graphic artifacts, high cpu usage of GPU process, etc;
* `brave://media-internals` in case of problems during video playback.

6. Save a performance trace when the problem happens and attach it to the issue. https://www.chromium.org/developers/how-tos/submitting-a-performance-bug/

7. A screenshot of browser built-in Task Manager (`Menu` &rarr; `More tools` &rarr; `Task manager`)
