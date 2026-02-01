import os
import pandas as pd


def load_data(filepath: str) -> pd.DataFrame:
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.")
        exit(1)
    return pd.read_csv(filepath)


def clean_raw_data(df: pd.DataFrame) -> pd.DataFrame:
    """Preprocess raw dataframe: parse resolution, rename column."""
    df = df.copy()
    if "Width * Height" in df.columns:
        df = df.rename(columns={"Width * Height": "Pixel Count"})

    # Convert "Width * Height" string to integer pixel count
    def parse_resolution(res_str: str) -> int:
        try:
            w, h = map(int, str(res_str).split("*"))
            return w * h
        except ValueError:
            return 0

    if "Pixel Count" in df.columns:
        df["Pixel Count"] = df["Pixel Count"].apply(parse_resolution)

    return df
