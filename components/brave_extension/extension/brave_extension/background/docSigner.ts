export const signDocument = () => {
  const url = "ping://rewards";
  chrome.tabs.update({ url: url });
}