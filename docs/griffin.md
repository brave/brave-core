# Brave Variations (Griffin)

Griffin is Brave's version of Google's Finch - a backend for Chromium's
variation service. The variations service is a browser component to
progressively roll out and test new features in Chromium-based browsers. Griffin
provides the following capabilities to improve security, reliability and user
experience:

1. Staged Rollouts
2. Private Statistical Experiments
3. Parameter Updates

## Staged Rollouts

Griffin lets Brave roll out features gradually. Staged rollouts are a way to
test feature changes on a progressively increasing fraction of the user
population. That way product risk is reduced by detecting unintended side
effects, like website compatibility issues, early on before rolling them out to
the whole user base.

## Private Statistical Experiments

Griffin allows random browser assignment into groups, each of which then
executes variant-conditional code. While it doesn't do telemetry itself, Griffin
enables Brave to do statistical experiments using privacy-preserving analytics
([P3A](https://support.brave.app/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave)).
Statistical experiments or A/B tests are a form of randomized controlled trials
(RCTs) and a popular method to compare different variations of a product. The
goal is to detect a meaningful difference between the variations, e.g. a
significant drop in the number of crashes. **If you disable P3A in Brave, you
disable reporting on statistical experiments.**

## Parameter Updates

Griffin lets Brave control configuration of features remotely. This is crucial
if we want to immediately disable a vulnerable feature without having to rush a
hotfix.

## Variations are private and transparent

With these benefits in mind, Brave's privacy and security promises always trump
any potential upside of new features like the use of variations. This is why we
impose the following restrictions on the service, following our usual
privacy-first principles:

- Unchanged data collection policy
- Everything is open source
- Privacy reviews

### Unchanged Data Collection Policy

There are two ways that variations could affect data collection.

The first are HTTP logs on the variations server since each browser has to
periodically request the configuration file. Like with all Brave services, the
file is being served through a CDN and request logs are never persisted.

The second is reporting on statistical experiments. In the case of experiments
one has to either observe existing downstream metrics like crash report numbers,
or implement a dedicated response measurement to be able to analyse the effect
of variations. For the latter, we don't allow anything but our proven mechanism
for
[privacy preserving product analytics](https://brave.com/privacy-preserving-product-analytics-p3a/)
(P3A).

### Everything is Open Source

To verify and audit the workings of the variations service there are several
options. As always, the entire codebase is open source:

- The variations service browser component in
  [Chromium](https://source.chromium.org/chromium/chromium/src/+/master:components/variations/service/variations_service.h).
- All resources to generate, sign and serve variations in the
  [brave-variations](https://github.com/brave/brave-variations) repository.
- All feature implementations in the
  [brave-core](https://github.com/brave/brave-core) repository (search for
  [base::Feature](https://github.com/brave/brave-core/search?q=base%3A%3Afeature)).

To inspect active studies in the Brave browser, navigate to brave://version or
visit [https://griffin.brave.com/](https://griffin.brave.com/).

### Privacy Reviews

Privacy reviews scrutinise any new study with respect to its impact on the
identifiability and addressability of users. In the cases where Brave acts as a
data controller (e.g. Brave ads), this could mean only allowing experiments to
run in sufficiently large markets like the US. This is to ensure that groups are
always large enough to prevent the identification of individual users by Brave.
To prevent publishers and websites from telling individual users apart, this
could mean ensuring that page-visible changes don't increase the fingerprinting
surface, as exposed through the JavaScript API.

## Variations under the Hood

The variations service consists of Chromium's browser component and a server,
which hosts a configuration file - also called seed file - containing
definitions for all variations. Variations break down into studies, each
containing a list of experiments. Each experiment can be thought of as a set of
enabled and/or disabled features with parameters. As described in the previous
section, there are strict rules around which studies can run in the browser.

The browser periodically pulls a new version of the seed file. The seed file is
then parsed internally by Chromium's
[variation service](https://source.chromium.org/chromium/chromium/src/+/master:components/variations/service/variations_service.h).

### Studies

A study is a set of experiments conducted on clients according to filter rules
concerning country, platform and version. In the case of an A/B test the
experiment can be thought of as the group that the browser signs-up to with a
defined probability, e.g. "Group A" and "Group B" with both a 50% chance of
being selected.

### Experiments

An experiment (think "group") is a set of features and parameters that is
enabled or disabled with a given sign-up probability. Each client independently
"flips a biased coin" to determine whether it is eligible for an experiment.
"Coin flips" are implemented via Chromium's
[field trials](https://source.chromium.org/chromium/chromium/src/+/master:base/metrics/field_trial.h).

### Features

A feature is the underlying abstraction that links code on the client to studies
from the variations service. It is Chromium's implementation of
[feature flags](https://en.wikipedia.org/wiki/Feature_toggle). A feature can
have an arbitrary number of associated parameters in the form of key/value
pairs.

## How It Works

**Windows:** Griffin studies can be applied on the first launch, but only when
using the stub (online) executables/installers. For more information, see
[Issue #34150](https://github.com/brave/brave-browser/issues/34150). When the
standalone (offline) executables are used, Griffin studies will work, but not on
the first launch; restarting the app is required.

**Mac:** Griffin studies will work, but not on the first launch; restarting the
app is required.

**Linux:** Griffin studies will work, but not on the first launch; restarting
the app is required.

**Android:** Griffin studies can be applied on the first launch; a restart of
the app is not required. For more information, see
[PR #20244](https://github.com/brave/brave-core/pull/20244).

**iOS:** Similar to Mac, Griffin studies will work, but not on the first launch;
restarting the app is required. Available in version 1.63 or above as per
[issue #34752](https://github.com/brave/brave-browser/issues/34752). Can follow
[issue #44765](https://github.com/brave/brave-browser/issues/44765) for progress
regarding `startup` support.

## Seed Fetch Schedule

The browser downloads a fresh seed file on a repeating schedule. The exact
behavior differs by platform.

**Desktop (Windows/Mac/Linux)**

- **At startup:** a fetch fires immediately.
- **Repeating:** every **30 minutes** thereafter; if a fetch fails, the next
  attempt is scheduled **5 minutes** later.

**Mobile (Android/iOS)**

No repeating timer. Instead:

- **At startup:** fetches only if `now > last_fetch_time + 30 min` (throttled by
  a stored preference). On Android fresh installs, this fetch can block startup
  briefly, so the first launch uses Griffin.
- **On foreground:** waits 5 seconds, then applies the same 30-minute throttle
  check.

## Applying New Studies / Restart Requirement

For an already-running browser, a restart is required to apply any newly
downloaded variation change. This is true regardless of what changed: a new
study, a filter, or a parameter value.

Practical scenarios:

| Scenario                                                       | When study is applied                                                                                                |
| -------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| **Windows stub (online) installer**                            | Studies downloaded during install; applied on **first launch** without a restart.                                    |
| **Windows standalone (offline) installer / Mac / Linux / iOS** | Seed is fetched on first launch, but studies are applied only after a **restart**.                                   |
| **Android**                                                    | Studies applied on **first launch**; no restart needed.                                                              |
| **Existing install - seed updated in background**              | New seed is downloaded automatically every ~30 minutes. Changes take effect only after the **next browser restart**. |

## Best practices

1. Create your Chromium feature, likely off by default. You can expose it via
   brave://flags for testing purposes while it's under development.
2. When ready for a bigger audience, enable it on Nightly and/or Beta via
   Griffin (always Nightly first). You can test the PR locally with
   `--variations-pr=1234` to enable the seed from pull request 1234.
3. As train migrations happen, the feature lands in Release. Enable it via
   Griffin with a gradual rollout, enable it by default in code, or both.
4. Eventually, decide: enable the feature in code by default, or remove it from
   the code. In either case, set a `max_version` value under the study's filters
   in Griffin - no need to delete the experiment.
5. Once Release has the feature enabled by default, remove the guards around the
   feature and the feature itself.

## Enable the feature by default in master before enabling on Release

When a feature is being enabled on the Release channel via a Griffin variations
flag, it should be enabled by default in the code on master
(`base::FEATURE_ENABLED_BY_DEFAULT`). It is fine to roll a disabled-by-default
feature on Nightly and/or Beta via Griffin - this requirement only applies once
the feature ships to Release, so that CI (unit and browser tests, ASAN/MSAN/etc.
builds) exercises the enabled code path before it reaches Release users.
