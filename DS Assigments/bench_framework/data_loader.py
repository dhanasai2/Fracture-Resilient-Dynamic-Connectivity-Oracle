import glob
import pandas as pd
import os

AVAILABLE = {
    'random': 'random_integers_100k.csv',
    'nearly_sorted': 'nearly_sorted_100k.csv',
    'duplicate_heavy': 'duplicate_heavy_100k.csv',
    'sorted': 'sorted_100k.csv',
    'skewed': 'skewed_powerlaw_100k.csv',
    'students': 'student_records_50k.csv',
}


def list_csvs(root=None):
    root = root or os.getcwd()
    return sorted(glob.glob(os.path.join(root, "*.csv")))


def load_dataset(key_or_path, n=None):
    # allow key mapping or direct path
    path = AVAILABLE.get(key_or_path, key_or_path)
    if not os.path.isabs(path):
        path = os.path.join(os.getcwd(), path)
    df = pd.read_csv(path, nrows=n)
    # choose first column as key column
    if df.shape[1] == 0:
        return pd.Series(dtype=object)
    return df.iloc[:, 0]
