from config import *
import pandas as pd
import numpy as np
import joblib
import jinja2
from sklearn.model_selection import train_test_split
from sklearn.pipeline import Pipeline
from sklearn.pipeline import FeatureUnion
from sklearn.base import BaseEstimator, TransformerMixin
from sklearn.compose import ColumnTransformer
from sklearn.preprocessing import StandardScaler
from sklearn.preprocessing import PolynomialFeatures
from sklearn.feature_selection import SelectKBest
from sklearn.feature_selection import mutual_info_regression
from sklearn.feature_selection import SelectFromModel
from sklearn.model_selection import GridSearchCV
from sklearn.metrics import (r2_score, mean_squared_error)

from sklearn.linear_model import (LassoCV, Lasso, Ridge, ElasticNet, HuberRegressor)

REGRESSOR = Lasso()

class ColumnExtractor(BaseEstimator, TransformerMixin):
    def __init__(self, columns=None):
        self.columns = columns
    def fit(self, X, y=None):
        return self
    def transform(self, X):
        X_cols = X[self.columns]
        return X_cols

def _load_dataset(path):
    """
    Load dataset
    """
    # Load data and reset index
    df = pd.read_csv(path, index_col='url', sep='\t')
    df = df.reset_index(drop=True)
    df = df.drop(columns=['experiment', 'metrics.interactive'])

    # Log transform target
    df['adblockSummary.wastedBytes_target_log10'] = np.log10(
        df['adblockSummary.wastedBytes_target'].values)

    # Define columns
    top_level_cols = ['adblockRequests']
    third_party_cols = [c for c in df.columns if 'thirdParties.' in c]
    metrics_cols = list(set([c for c in df.columns if 'metrics.' in c]) - set(['metrics.firstCPUIdle', 'metrics.observedLastVisualChange']))
    resources_cols = [c for c in df.columns if 'resources.' in c]
    diagnostics_cols = [c for c in df.columns if 'diagnostics.' in c]
    dom_cols = [c for c in df.columns if 'dom.' in c]
    critical_request_chains_cols = [c for c in df.columns
                                    if 'criticalRequestChains.' in c]
    target_cols = [c for c in df.columns if 'adblockSummary.' in c]
    misc_cols = df.columns.difference(top_level_cols + third_party_cols + metrics_cols +
                                      resources_cols + diagnostics_cols +
                                      dom_cols + critical_request_chains_cols +
                                      target_cols).to_list()

    cat_cols = third_party_cols
    num_cols = df.columns.difference(cat_cols + target_cols + misc_cols + diagnostics_cols +
        dom_cols + critical_request_chains_cols).to_list()

    y_col = 'adblockSummary.wastedBytes_target_log10'
    x_cols = cat_cols + num_cols

    # Split in train and test set
    X_train, X_test, y_train, y_test = train_test_split(df[x_cols],
                                                        df[y_col],
                                                        test_size=TEST_SIZE,
                                                        random_state=RANDOM_STATE)

    print("Dataset: X_train, X_test: (%i, %i) samples with (%i) features"
          % (X_train.shape[0], X_test.shape[0], X_train.shape[1]))

    return X_train, X_test, y_train, y_test, cat_cols, num_cols


def _build_model(num_cols, cat_cols, reg):
    """
    Build model
    """
    polynomials = PolynomialFeatures(interaction_only=False)

    # num_col_processor = Pipeline([
    #     ("standardise", StandardScaler()),
    #     ('polynomials', polynomials)
    # ])

    pre_processor = ColumnTransformer([
        # ('num_cols_processor', num_col_processor, num_cols),
        ("standardise", StandardScaler(), num_cols),
        ("pass_through", "passthrough", cat_cols)
    ])

    feature_selector = SelectKBest(mutual_info_regression)
    # feature_selector = SelectFromModel(LassoCV(cv=N_FOLDS), threshold=0.0001)

    model = Pipeline([
        ("pre_processor", pre_processor),
        ("feature_selector", feature_selector),
        ("model", reg)
    ])

    return model


def tune_model(print_params=False, reg=REGRESSOR):
    """
    Tune Model
    """
    X_train, _, y_train, _, cat_cols, num_cols = _load_dataset(DATA_PATH)

    # Instantiate model to tune
    model = _build_model(num_cols, cat_cols, reg)

    # Run grid search
    gs = GridSearchCV(model, GRID_PARAMS, scoring='r2', n_jobs=-1, cv=N_FOLDS)
    gs.fit(X_train, y_train)

    print("Best score: ", gs.best_score_)

    if print_params:
        print("Best Hyperparameters: {}".format(gs.best_params_))

    return gs.best_params_


def train_model(print_params=False, reg=REGRESSOR):
    """
    Train Model
    """
    X_train, _, y_train, _, cat_cols, num_cols = _load_dataset(DATA_PATH)

    # Instantiate model and set best parameters from grid search
    model = _build_model(num_cols, cat_cols, reg)
    model = model.set_params(**BEST_PARAMS)

    model.fit(X_train, y_train)

    # Save model
    joblib.dump(model, MODEL_PATH)
    print('Model dumped at ' + MODEL_PATH)

    # Return parameters for diagnosis
    if print_params:
        print(model.get_params())

    return model.get_params()

def export_model():
    # Load trained model and predict on test set
    model = joblib.load(MODEL_PATH)
    # Export:
    transformers = {
        'standardise': {
            'features': [],
            'feature_map': {},
            'mean': [],
            'scale': []
        },
        'passthrough': {
            'features': []
        }
    }
    for (name, _, features) in model['pre_processor'].transformers:
        transformer = model['pre_processor'].named_transformers_[name]
        # print(transformer)
        if name == 'standardise':
            transformers['standardise']['mean'] = transformer.mean_
            transformers['standardise']['scale'] = transformer.scale_
            transformers['standardise']['features'] = features
            transformers['standardise']['feature_map'] = list(zip(features, zip(transformer.mean_, transformer.scale_)))
        elif name == 'pass_through':
            transformers['passthrough']['features'] = features
        else:
            raise Exception('Unexpected pre_processor transformer: {}'.format(name))

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(EXPORT_TEMPLATE_PATH), trim_blocks=True, lstrip_blocks=True)
    data = {
        'transformers': transformers,
        'model': {
            'intercept': model['model'].intercept_,
            'coefficients': model['model'].coef_
        },
        'misc': {
            'entities': [ feature.replace('thirdParties.', '').replace('.blocked', '') for feature in transformers['passthrough']['features'] if feature.startswith('thirdParties.') ]
        }
    }
    env.get_template(EXPORT_TEMPLATE_NAME).stream(data).dump(EXPORT_OUTPUT_PATH)


def test_model():
    """
    Test Model
    """
    _, X_test, _, y_test, cat_cols, num_cols = _load_dataset(DATA_PATH)

    # Load trained model and predict on test set
    model = joblib.load(MODEL_PATH)

    y_pred = model.predict(X_test)

    print('r^2', r2_score(y_test, y_pred))
    print('MSE', mean_squared_error(y_test, y_pred))

    return y_test, y_pred

def predict():
    pass
