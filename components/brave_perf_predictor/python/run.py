from config import *
import argparse
from model import (tune_model, train_model, test_model, export_model)


def main():
    parser = argparse.ArgumentParser(
        description="CLI tool to manage project"
    )

    parser.add_argument(
        'stage',
        metavar='STAGE',
        type=str,
        choices=['tune', 'train', 'test', 'export', 'predict', 'algorithm_spotcheck',
                 'diagnose_model'],
        help='Select pipeline stage to run: %(choices)s'
    )

    stage = parser.parse_args().stage

    if stage == 'tune':
        tune_model(print_params=True)

    elif stage == 'algorithm_spotcheck':
        algorithm_spotcheck()

    elif stage == 'diagnose_model':
        diagnose_model()

    elif stage == 'train':
        train_model()

    elif stage == 'test':
        test_model()

    elif stage == 'export':
        export_model()

    elif stage == 'predict':
        predict()


if __name__ == "__main__":
    main()
