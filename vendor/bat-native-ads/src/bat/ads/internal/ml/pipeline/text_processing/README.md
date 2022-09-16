# Machine Learning Text Processing Pipeline

Pipeline instructions specifically related to processing model input text.

## Embedding Processing

In `EmbeddingProcessing::EmbedText`, the text to embed is gathered from a web page's [og:title](https://developers.facebook.com/docs/sharing/webmasters/) HTML tag, if available. The length of the title text should typically be about one sentence long.
