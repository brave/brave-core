# Dirs and files
BASE_URL = './'
RESOURCES_DIR = BASE_URL + 'resources/'
DATA_PATH = RESOURCES_DIR + 'df.tsv'
MODEL_DIR = RESOURCES_DIR
MODEL_NAME = 'model.joblib'
MODEL_PATH = MODEL_DIR + MODEL_NAME
EXPORT_OUTPUT = 'bandwidth_linreg_parameters.h'
EXPORT_OUTPUT_PATH = BASE_URL + '../browser/' + EXPORT_OUTPUT
EXPORT_TEMPLATE_NAME = EXPORT_OUTPUT + '.tpl'
EXPORT_TEMPLATE_PATH = RESOURCES_DIR

# Train/Test configuration
TEST_SIZE = .2
RANDOM_STATE = 42
N_FOLDS = 5

# Best params for Bandwidth with Lasso regressor
BEST_PARAMS ={
    'feature_selector__k': 'all',
    'model__alpha': 0.00025,
    'model__fit_intercept': True,
    'model__max_iter': 10000,
    'model__normalize': True,
    'model__selection': 'random',
    'pre_processor__standardise__with_mean': True,
    'pre_processor__standardise__with_std': True
}

GRID_PARAMS = {
    'pre_processor__standardise__with_mean': [True],
    'pre_processor__standardise__with_std': [True],
    # 'pre_processor__num_cols_processor__polynomials__degree': [1],
    'feature_selector__k': ['all'],
    # 'feature_selector__threshold': [0.0001, 0.001, 0.00001, 0],
    'model__alpha': [0.0005, 0.00025, 0.0001],
    'model__normalize': [True],
    'model__max_iter': [10000, 20000, 30000],
    'model__fit_intercept': [True],
    'model__selection': ['random']
}
