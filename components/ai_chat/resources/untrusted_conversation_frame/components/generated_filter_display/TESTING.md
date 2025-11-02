# Testing the GeneratedFilterDisplay Component

This guide explains how to test the GeneratedFilterDisplay UI component before the backend integration is complete.

## Option 1: Visual Preview in Browser (Recommended)

### Step 1: Enable the Preview Component

Temporarily edit `conversation_entries/index.tsx` to show preview examples:

1. Open `/components/ai_chat/resources/untrusted_conversation_frame/components/conversation_entries/index.tsx`

2. Add this import at the top:
```typescript
import GeneratedFilterPreview from '../generated_filter_display/preview'
```

3. Add the preview component in the render section (around line 180-200):
```typescript
return (
  <div className={styles.entries}>
    {/* TEMPORARY: Preview generated filter displays */}
    <GeneratedFilterPreview />

    {groupedEntries.map((group, i) => {
      // ... existing code
```

4. Rebuild:
```bash
build
```

5. Launch Brave and open Leo (AI Chat sidebar)

6. You should see 4 example filter cards displayed showing:
   - CSS selector with high confidence
   - Scriptlet with medium confidence
   - Multiple target elements with low confidence
   - Complex scriptlet with MutationObserver

7. **Remember to remove the preview import and component before committing!**

---

## Option 2: Unit Tests

Run the unit tests to verify component rendering:

```bash
npm run test -- brave_unit_tests --filter=GeneratedFilterDisplay*
```

The tests verify:
- CSS filter rendering
- Scriptlet filter rendering
- Confidence level badges
- Multiple target elements
- Debug notes for scriptlets

---

## Option 3: Mock Data in Backend (Advanced)

If you want to test with real conversation flow, you can temporarily inject a mock GeneratedFilterEvent in the backend:

### Edit `conversation_handler.cc`

Find `OnEngineCompletionComplete` (around line 1484) and add this temporary code:

```cpp
void ConversationHandler::OnEngineCompletionComplete(
    EngineConsumer::GenerationResult result) {
  // ... existing code ...

  // TEMPORARY: Add a mock generated filter event for testing
  if (chat_history_.back()->character_type == mojom::CharacterType::ASSISTANT) {
    auto filter_event = mojom::GeneratedFilterEvent::New();
    filter_event->filter_type = mojom::GeneratedFilterType::CSS_SELECTOR;
    filter_event->domain = "example.com";
    filter_event->code = "example.com##.cookie-banner";
    filter_event->description = "Hides the cookie consent banner";
    filter_event->target_elements = std::vector<std::string>{".cookie-banner"};
    filter_event->confidence = "high";
    filter_event->reasoning = "The class name clearly indicates this is a cookie banner.";

    auto event = mojom::ConversationEntryEvent::NewGeneratedFilterEvent(
        std::move(filter_event));
    chat_history_.back()->events->push_back(std::move(event));
  }

  // ... rest of existing code ...
}
```

Then rebuild and any Leo response will include the filter card.

**Remember to remove this mock code before committing!**

---

## What to Look For

When testing, verify:

### Visual Elements
- ✅ Card has rounded corners and proper padding
- ✅ Header shows correct icon (palette for CSS, code for scriptlet)
- ✅ Confidence badge has correct color (green/orange/red)
- ✅ Code block has syntax highlighting
- ✅ Copy button appears in code block toolbar
- ✅ Debug note only appears for scriptlets
- ✅ Domain info shows at bottom

### Color Coding
- ✅ High confidence: Green badge with checkmark icon
- ✅ Medium confidence: Orange badge with warning icon
- ✅ Low confidence: Red badge with triangle icon

### Content Sections
- ✅ Description is clear and readable
- ✅ Target elements appear as inline code badges
- ✅ Reasoning is in lighter gray text
- ✅ Filter code is properly formatted

### Responsiveness
- ✅ Component adapts to different widths
- ✅ Target element badges wrap properly
- ✅ Code block scrolls horizontally if needed

---

## Next Steps After Testing

Once you verify the UI looks good:

1. Remove any temporary preview/mock code
2. Proceed with backend integration to create real GeneratedFilterEvent objects
3. Test end-to-end with actual filter generation requests

---

## Troubleshooting

### Component doesn't appear
- Check browser console for errors
- Verify build completed successfully
- Make sure Mojo bindings regenerated (`gen/brave/components/ai_chat/core/common/mojom/`)

### Styling looks wrong
- Check that Leo design tokens are loaded
- Verify CSS modules are working (should see scoped class names)
- Try hard refresh (Cmd+Shift+R)

### TypeScript errors
- Run `npm run build` in the resources directory
- Check that all imports are correct
- Verify mojom TypeScript bindings generated correctly
