# Prerequisites

You don't need to do a full brave browser build, but you do need some generated types and strings via a much much smaller build (make sure to sync first):

```bash
npm run build -- --target=brave:ai_chat_app_lib
```

# Work locally in a browser
```bash
# Runs webpack, watching for changes, and a web server to view the site
npm run ai-chat-app-dev
```

Then load http://localhost:8081 in any browser.

Any edits to any TS, CSS or HTML will be automatically re-compiled and the browser will automatically refresh (not hot-reload).

# Deploy to an app
First make sure you're running the above `npm run ai-chat-app-dev` command, or for production compilation `npm run ai-chat-app-prod`.

```
npx cap run ios

# or

npx cap run android
```

That will compile and open the app on an emulator or device of your choice.
