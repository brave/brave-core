const puppeteer = require("puppeteer");
const fs = require("fs");
const path = require("path");

// Define the list of URLs to navigate to
const urls = [
  "https://twitter.com",
  "https://github.com",
  "https://youtube.com",
  "https://reddit.com",
];

// Keywords indicative of rewards-related requests
const rewardsKeywords = [
  "ledger.mercury.basicattentiontoken.org",
  "ledger-staging.mercury.basicattentiontoken.org",
  "balance.mercury.basicattentiontoken.org",
  "balance-staging.mercury.basicattentiontoken.org",
  "publishers.basicattentiontoken.org",
  "publishers-staging.basicattentiontoken.org",
  "publishers-distro.basicattentiontoken.org",
  "publishers-staging-distro.basicattentiontoken.org",
];

// Function to check if a URL is related to rewards
const isRewardsRelated = (url) =>
  rewardsKeywords.some((keyword) => url.includes(keyword));

// Function to navigate to each URL and monitor network requests
const monitorNetworkRequests = async () => {
  const browser = await puppeteer.launch({ headless: true });
  const page = await browser.newPage();

  // Enable request interception to monitor network requests
  await page.setRequestInterception(true);
  page.on("request", (interceptedRequest) => {
    const requestUrl = interceptedRequest.url();
    console.log(`Request URL: ${requestUrl}`);
    if (isRewardsRelated(requestUrl)) {
      console.error(`Rewards related request found: ${requestUrl}`);
      process.exit(1); // Fail the test if a rewards related request is found
    }
    interceptedRequest.continue();
  });

  try {
    // Navigate to each URL and wait for the page to load
    for (const url of urls) {
      console.log(`Navigating to: ${url}`);
      await page.goto(url, { waitUntil: "networkidle0", timeout: 60000 });
    }
  } catch (error) {
    console.error("Error during navigation:", error);
    await browser.close();
    process.exit(1);
  }

  await browser.close();
  console.log(
    "Test completed successfully. No rewards related requests found."
  );
};

monitorNetworkRequests().catch((error) => {
  console.error("Error during network requests monitoring:", error);
  process.exit(1);
});
