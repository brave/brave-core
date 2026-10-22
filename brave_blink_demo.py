#!/usr/bin/env python3
"""Launch the Brave iOS Blink build in the simulator.

Usage: ./brave_blink_demo.py [--build] [--onboarding] [--showcase|--serve] [URL]
"""

import argparse
import subprocess
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

SIM = "B414BC85-5A80-4803-B360-596083E53F9F"
BUNDLE_ID = "com.brave.ios.BrowserBeta"
BRAVE_DIR = Path(__file__).resolve().parent
DERIVED_DATA = "/tmp/brave_blink_dd"
APP = f"{DERIVED_DATA}/Build/Products/Debug-iphonesimulator/Client.app"
SHOWCASE_PORT = 8932
LION_SVG = ("ios/brave-ios/Sources/BraveUI/Images/Shared.xcassets/"
            "Colored PDF/brave.logo.imageset/brave.logo.svg")


def run(cmd, cwd=None, check=True):
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, check=check)


def build():
    run(["pnpm", "run", "build", "Static", "--channel=nightly",
         "--target_os=ios", "--target_arch=arm64",
         "--target_environment=simulator", "--target=brave:all",
         "--gn=use_blink:true", "--gn=is_asan:false"], cwd=BRAVE_DIR)
    run(["xcodebuild", "-project", "Client.xcodeproj",
         "-scheme", "Static (No Core)", "-configuration", "Debug",
         "-destination", f"id={SIM}", "-derivedDataPath", DERIVED_DATA,
         "build"], cwd=BRAVE_DIR / "ios/brave-ios/App")


def showcase_server():
    # Serves only these two files, never the rest of the repo.
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/lion.svg":
                body = (BRAVE_DIR / LION_SVG).read_bytes()
                kind = "image/svg+xml"
            else:
                body = (BRAVE_DIR / "brave_blink_demo.html").read_bytes()
                kind = "text/html; charset=utf-8"
            self.send_response(200)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *args):
            pass

    return HTTPServer(("127.0.0.1", SHOWCASE_PORT), Handler)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true",
                        help="rebuild core and the app, then reinstall")
    parser.add_argument("--onboarding", action="store_true",
                        help="show onboarding instead of skipping it")
    parser.add_argument("--showcase", action="store_true",
                        help="serve and open the Blink feature showcase page")
    parser.add_argument("--serve", action="store_true",
                        help="serve the showcase page, but open a new tab")
    parser.add_argument("url", nargs="?", help="page to open after launch")
    args = parser.parse_args()

    showcase = f"http://localhost:{SHOWCASE_PORT}/"
    server = None
    if args.showcase or args.serve:
        server = showcase_server()
    if args.showcase:
        args.url = showcase

    if args.build:
        build()

    # bootstatus -b boots the simulator if needed and waits until it's ready.
    run(["xcrun", "simctl", "bootstatus", SIM, "-b"])
    run(["open", "-a", "Simulator"])

    if args.build:
        run(["xcrun", "simctl", "install", SIM, APP])

    # Stored as defaults rather than launch args so they survive the relaunch
    # that opening a URL can trigger.
    for flag in ["BRSkipOnboarding", "BRSkipEduPopups", "BRSkipAppLaunchPopups"]:
        run(["xcrun", "simctl", "spawn", SIM, "defaults", "write", BUNDLE_ID,
             flag, "-bool", "NO" if args.onboarding else "YES"])

    run(["xcrun", "simctl", "launch", "--terminate-running-process",
         SIM, BUNDLE_ID])

    if args.url or args.serve:
        from urllib.parse import quote
        time.sleep(8)
        # Without a url, open-url opens a New Tab Page.
        link = "brave-debug://open-url"
        if args.url:
            link += "?url=" + quote(args.url, safe="")
        run(["xcrun", "simctl", "openurl", SIM, link])
        print("Click 'Open' on the prompt in the simulator.")

    if server:
        print(f"Serving the showcase on {showcase} (Ctrl-C to stop).")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
