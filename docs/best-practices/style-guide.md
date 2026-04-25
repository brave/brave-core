# Brave Style Guide

Best practices for all user-facing text in the Brave browser, derived from the Brave Style Guide. These rules apply to UI strings, settings, error messages, tooltips, notifications, and all other surfaces.

See also: [Localization & String Resources](./localization.md) for GRD/GRDP technical conventions.

---

<a id="SG-001"></a>

## ✅ Voice: Concise, Descriptive, Simple, Active

**All user-facing text must use active voice, be concise, and aim for simplicity.**

- Write in active voice, not passive. Tell us who did something, not who it was done by.
- Aim for a fifth-grade reading level in general product UI.
- Be honest and friendly, but not jokey or whimsical.
- Avoid jargon unless targeting a technical audience, and define terms at first usage.

```
❌ "Your browsing history has been cleared by the system."
✅ "Brave cleared your browsing history."

❌ "The page could not be loaded due to a network connectivity issue."
✅ "This page failed to load. Check your connection and try again."
```

---

<a id="SG-002"></a>

## ✅ Use Contractions in UI Text

**Use contractions where possible.** They're more conversational and save space in UI strings.

```
❌ "You cannot undo this action."
✅ "You can't undo this action."

❌ "This is not a secure connection."
✅ "This isn't a secure connection."
```

---

<a id="SG-003"></a>

## ❌ Avoid Colloquial Language, Idioms, and Slang

**Do not use colloquial language, idioms, or slang in UI strings.** They can be confusing, offensive, or difficult to translate.

Also avoid overused filler words like "basically," "obviously," "literally," or "frankly."

```
❌ "This ad looks creepy"
✅ "This ad looks suspicious"
```

---

<a id="SG-004"></a>

## ✅ Use Second-Person Perspective; First-Person for User Actions

**Address readers using second person (you, your). When a user is asked to take an action or give consent, use first person (I, me, my).**

```
✅ "Log in to your Brave account"
✅ "I've read the Brave terms of service"
```

Avoid repetition of "your" — e.g., "Log in to your Premium account to manage subscriptions" instead of "...to manage your subscription."

When discussing legal, security, or data privacy, use "Brave" or "the Brave team" instead of "we" to avoid implying staff can access user data.

```
✅ "Brave can only access the following search data…"
✅ "Brave blocked this page because it appears to have hidden trackers"
```

---

<a id="SG-005"></a>

## ✅ Keep Sentences Short (15 Words or Fewer)

**Aim for 15 words or fewer per sentence.** Break long sentences into two, or use bullet points. Test by reading aloud.

```
❌ "Are you sure you want to put Shields down for this page? Shields down may reduce your privacy."
✅ Title: "Put Shields down?"
   Body: "Shields down may reduce your privacy on this site."
```

---

<a id="SG-006"></a>

## ✅ Use Simple Present Tense by Default

**Default to simple present tense when there's no negative impact.** Avoid progressive (-ing) and complex verb constructions.

Check: if the verb is preceded by was/were, has/have, is/are, or be, or ends in -ing, it's not simple tense.

```
❌ "Once you click 'Search default,' you'll see a list of search options"
✅ "Click 'Search default' to see a list of search options"

❌ "Clicking 'Search default' will show a list of search options"
✅ "Click 'Search default' to see a list of search options"
```

---

<a id="SG-007"></a>

## ✅ Use Gender-Neutral Language

**Rephrase sentences to avoid gendered pronouns. If unavoidable, default to they/their.** Also avoid gender-latent words like "policeman" or "stewardess" — use "police officer" or "flight attendant."

---

<a id="SG-008"></a>

## ✅ Sentence Case by Default; Title Case Only for Product Names

**Default to sentence case in almost all contexts:** headlines, titles, buttons, email subject lines, Mac file system warnings, Windows warnings.

**Never use ALL CAPS.**

Title case is used only for branded product/plan names (see [SG-014](#SG-014)).

Exception: iOS uses title case on UI copy and settings per Apple's platform rules, which override Brave's defaults.

---

<a id="SG-009"></a>

## ✅ Use the Oxford (Serial) Comma

**Always place a comma before the last item in a list of 3 or more items.**

```
❌ "Keep your friends, Brendan and Brian, safe with Brave"
✅ "Keep your friends, Brendan, and Brian safe with Brave"
```

---

<a id="SG-010"></a>

## ✅ Punctuation Rules

**Follow these punctuation conventions in all user-facing text:**

- **Ampersands (&):** Don't use to replace "and." Exception: "Trackers & ads" is acceptable to highlight their close relationship.
- **Bullet points:** No periods at end, unless any item has more than one sentence (then all items get periods).
- **Colons:** Capitalize after a colon only if a complete sentence follows.
- **Ellipsis:** Use the Unicode character `…` (option-semicolon on Mac), not three periods.
- **Em dashes (—):** Use option-shift-hyphen. No spaces before or after.
- **En dashes (–):** Use for date/time spans replacing "to." E.g., "Monday–Friday."
- **Exclamation points:** Use sparingly, never more than one per view/post/email/article.
- **Hyphens:** Hyphenate multi-word adjectives before a noun ("up-to-date browser"), not after ("browser is up to date"). Use hyphens for repeated-letter words ("re-enable"), but prefer writing around the usage.
- **Periods:** Include at end of body copy and numbered list items. Do not include at end of titles, headings, subheads, bullet points, or buttons. One space after periods.
- **Quotation marks:** Use double quotes when referring to UI copy in instructions (e.g., Click "Search options"). Commas, exclamation points, and periods go inside quotation marks.

---

<a id="SG-011"></a>

## ✅ UI Copy: Lead with Action, Be Brief

**In UI copy, start with imperative verbs and omit extraneous words.**

```
❌ "Be sure to select the address that matches your cryptocurrency type."
✅ "Select the address that matches your cryptocurrency type."

❌ "Would you like to delete your history now?"
✅ "Delete history?"

❌ "You can create an account"
✅ "Create an account"
```

**Buttons:**
- 3 words or fewer, sentence case (never all caps)
- Lead with a verb describing the action
- Omit articles (the, a, an)

**Links:**
- Never use "click here" by itself
- Use "click" for desktop, "tap" for mobile apps
- Use "Learn more" with context: "Learn more about Brave Rewards"

**Settings toggles:**
- Label the action in the positive: "Show Home button" not "Hide Home button"

**Error messages:**
- Always include what happened AND a solution or way to contact support
- Never a dead end
- Avoid: exclamations ("Whoopsie!"), apologies, "please," blame, technical jargon (server, fetch, abort, illegal, cache, invalid), empty promises, new concepts

```
✅ "This page failed to load. Try reloading, or setting Shields down."
✅ "Zero results for your query. Try a new query with different keywords."
```

**Confirmation modals:**
- **Title:** State the action concisely with a question mark
- **Body:** State the consequences concisely
- **Button:** Repeat the action verb

```
Title: "Set Brave as your default browser?"
Body:  "Any web links you click will open in the Brave browser."
Button: [Set as default]
```

---

<a id="SG-012"></a>

## ✅ Descriptive Links for Accessibility

**Never use "click here" as link text. Instead, describe what happens when clicked.** This is critical for screen reader users.

```
❌ "To unsubscribe from these emails, click here."
✅ "Want to stop getting these emails? Unsubscribe."
```

Also:
- Add descriptive text to images and visuals — don't rely solely on images/colors/animations
- Avoid directional language ("to the right of," "above," "below") — include additional description when unavoidable

---

<a id="SG-013"></a>

## ✅ Numbers, Currency, and Formatting

**Follow these formatting rules for numbers:**

| Range | Marketing body | UI / headlines |
|-------|---------------|----------------|
| 0–9 | Spell out ("three") | Digits ("3") |
| 10–999 | Digits everywhere | Digits everywhere |
| 1,000+ | Digits with commas | Digits with commas |

- Always spell out numbers that start a sentence
- Percentages: digits + % with no space (e.g., "50%")
- Measurement: space between number and unit (e.g., "1 TB", "100 GB")
- Currency: use symbol for the country (e.g., "$9.99"), specify country outside US (e.g., "HK$9.99")
- Dates: no ordinal numbers ("June 1" not "June 1st"), no numeric format ("6/1/21")
- Time: "6:00 am" / "12:15 pm" (lowercase, space before am/pm)
- Fractions: always spell out "half"; others as x/y without ordinals

---

<a id="SG-014"></a>

## ✅ Product and Feature Naming Conventions

**Use these exact names for Brave products and features (title case):**

- Basic Attention Token
- Brave Ads / Brave Private Ads
- Brave browser (lowercase "browser")
- Brave Firewall + VPN
- Brave Leo AI
- Brave News
- Brave Playlist
- Brave Premium
- Brave Rewards
- Brave Search
- Brave Shields
- Brave Swap
- Brave Talk / Brave Talk Premium
- Brave Wallet
- Brave Creators
- Ad-free Brave Search (or "ad-free Brave Search" mid-sentence)

**Rules:**
- "Brave browser" always lowercase "browser" (legacy convention)
- On first usage or when referring to the branded feature, use the full name
- For general/informal usage, lowercase is fine: "create a playlist," "check your news feed," "manage your rewards"
- **Exception:** Shields is always capitalized, even in general usage
- "Brave" is always capitalized except when referring to the URL "brave.com"
- Never use "Brave" as an adjective (don't say "be brave" or "be Brave")
- Use "Brave browser" instead of just "Brave" where possible to avoid associating the entire brand with just the browser

---

<a id="SG-015"></a>

## ❌ Words and Phrases to Avoid

**Do not use these words/phrases in user-facing text:**

| Don't use | Use instead |
|-----------|-------------|
| "Brave ads" / "private ads" (lowercase) | "Brave Ads" or "Brave Private Ads" |
| "be brave" / "be Brave" | (avoid entirely) |
| "donate" / "gift" / "give" / "pay" / "send" (for BAT contributions) | "contribute" or "support" |
| "enable" / "disable" | "turn on" / "turn off" |
| "watch" / "see" (for content) | "view" |
| "annually" / "monthly" (for subscriptions) | "per month" / "per year" |
| "tipping" (for Rewards) | "contributing" / "supporting" |
| "download" / "save" (for Playlist) | "add" |
| "e-mail" | "email" |
| "emojis" (plural) | "emoji" |
| "click here" | Descriptive link text |
| "smartphone" | "phone" |

---

<a id="SG-016"></a>

## ✅ Privacy and Security Terminology

**Privacy and security claims require specific, accurate language. Vague claims create legal and trust risks.**

- **"Private":** Describe specifically what data is private, how, and who has access.
- **"Secure":** Describe specifically what has been secured and how.
- **"Anonymous" / "Anonymity":** Almost never use. Get privacy team review. Reserve for cases where it's practically impossible for any adversary (including Brave) to identify a person.
- **"Tor":** Never written as "TOR." Don't make privacy/security/anonymity claims about Tor windows without Security/Privacy team approval.

**Never use:**
- "Military-grade encryption" / "unbreakable encryption"
- "Completely private" / "100% anonymous" / "totally secure" / "unhackable" / "hacker-proof"
- Any superlative privacy/security claims

**Preferred terms:**
- "IP dropping server" or "proxy server" instead of "anonymizing server"

---

<a id="SG-017"></a>

## ✅ Localization Considerations for UI Text

**Write strings that translate well across languages:**

- **Allow for text expansion** — translated text is often longer than English. Consider how the UI accommodates expanded text.
- **Put the most important information first** — translations may be truncated. Avoid "Be sure to [verb]…"; just start with "[Verb]…"
- **Avoid letters in icons** — characters like "Aa" don't translate across scripts (e.g., Arabic).
- **Add text labels to icons** — makes icons more translatable and accessible.
- **Keep text and images separate** — avoid images containing readable text, since text must be translated separately.
- **Avoid possessive constructions** — "the content of this page" translates better than "this page's contents."

---

<a id="SG-018"></a>

## ✅ BAT (Basic Attention Token) Terminology

**Follow these rules when referencing BAT:**

- Spell out "Basic Attention Token (BAT)" on first usage, then use "BAT"
- BAT is a "token" or "crypto asset" — never a "point," "currency," or "cryptocurrency"
- BAT should not be discussed as having a cash value
- Never accompany BAT with a currency symbol ($, €)
- Format BAT balances to three decimal places: "80.000 BAT"
- A number followed by BAT denotes the amount: "Tip 1 BAT" (Note: in Rewards context, use "contribute" instead of "tip")

---

<a id="SG-019"></a>

## ✅ Describing Devices

**Use the correct terminology when referring to user devices:**

- **Mobile:** Use "phone or tablet" instead of "device" or "mobile device" on first reference. Use actual device names (iPhone, iPad, Android) in subheads. Don't repeat "phone or tablet" more than once per paragraph — switch to "your device."
- **Desktop:** Use "computer" for non-mobile machines. Avoid "laptop," "PC," or "desktop" as they may exclude users on other form factors.

---

<a id="SG-020"></a>

## ✅ Product Messaging Constraints

**When writing about Brave products, follow these messaging rules:**

**Browser:**
- Do: mention speed (up to 3x faster than Chrome), Chrome extension support, 60-second import
- Don't: say Brave blocks ALL ads and trackers

**Rewards:**
- Do: say "contributing" / "supporting" / "getting paid"
- Don't: say "tipping" or "earning money"

**Playlist:**
- Do: say "add" files to a playlist, mention added media is ad-free
- Don't: say "download" / "save," or claim it works for "any" video/audio

**Search:**
- Do: mention independent index, anonymous community contributions, Goggles, Discussions
- Don't: (no specific restrictions beyond general accuracy)

**Talk:**
- Do: free video calls for up to 4 people, no time limit, no tracking, no extension/app/account needed
- Don't: say "E2E" or "end-to-end" encrypted

**VPN:**
- Do: access content on the go, free for 7 days, available on mobile and desktop
- Don't: say you can use it to watch streaming sites

**General:**
- Do: position as alternative to "Big Tech" / "ad tech"
- Don't: call out specific companies like Meta, or encourage activism
