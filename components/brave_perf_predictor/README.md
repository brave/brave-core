# Brave Performance Predictor

This component is based on our [blog post](https://brave.com/accurately-predicting-ad-blocker-savings/).

The current implementation is only for bandwidth prediction and uses a subset of the features discussed in the post for better cross-platform support. Key groups of features used are:

- URLs of blocked requests, from which we extract counts and distinct third-parties (e.g. tracking networks)
- total counts and sizes of loaded requests by type (scripts, images, fonts, stylesheets, media, document and “other”)
- basic page performance metrics (page load time, page interactive time, dom content loaded, first visual change, first meaningful paint)

The component includes:
- Python code that does model fitting and parameter tunning for data already provided in the expected format
- A small python script that translates the generated linear regression model to parameters in a C++ header file
- An interface to the model that buffers submitted features and runs the model when requested
