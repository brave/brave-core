
Bandwidth prediction model including training, tuning and exporting to a C++ header file.

Note: exporting is hand-tweaked to work for this model and not for any generic sklearn model.

To retrain the model for data provided in `resources/df.tsv`:

```
python run.py train
```

This will generate a binary model representation in `resources/model.joblib`. To then generate a header-file with the extracted model parameters:

```
python run.py export
```

Which will place the generated `predictor_parameters.h` file in `../browser/predictor_parameters.h`
