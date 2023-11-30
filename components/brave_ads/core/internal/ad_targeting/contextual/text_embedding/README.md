# Text Embedding Processor

This processor allows text data to be transformed into a vector that carries semantic meaning. The vector representation is created by averaging individual word vectors that are in-vocabulary (provided in the ads resources component). The word vectors themselves come from a pre-trained model. These vectors are intended to be used by downstream prediction tasks.

The text to embed is gathered from a web page's [og:title](https://developers.facebook.com/docs/sharing/webmasters/) HTML tag, if available. The length of the title text should typically be about one sentence long.
