# Text processing linear testing models

Linear models in this directory use FlatBuffers. Their schema is defined in
[text_classification_linear_transformation.fbs](../../../../../../internal/common/resources/flat/text_classification_linear_transformation.fbs).

Any model can be converted to a JSON representation by running FlatBuffer:

    flatc --json --strict-json --defaults-json --raw-binary text_classification_linear_model.fbs -- {FlatBuffers model path}
