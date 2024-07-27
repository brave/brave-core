# Text processing linear testing models

Linear models in the current directory are stored in a FlatBuffers representation with the schema defined at [text_classification_linear_transformation.fbs](//brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_transformation.fbs).

Any model can be converted to a JSON representation by running FlatBuffer:

    flatc --json --strict-json --defaults-json --raw-binary text_classification_linear_model.fbs -- {FlatBuffers model path}
