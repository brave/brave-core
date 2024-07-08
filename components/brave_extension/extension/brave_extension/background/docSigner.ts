export const signDocument = () => {
  const url = "ping://sign-pdf";
  chrome.tabs.update({ url: url });
}