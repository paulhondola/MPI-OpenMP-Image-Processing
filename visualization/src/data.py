import pandas as pd
import os


def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.")
        exit(1)
    return pd.read_csv(filepath)


def preprocess_data(df):
    # Melt the dataframe to handle multiple headers if necessary,
    # but the current structure has columns:
    # Pixel Count,Kernel Size,Clusters,Threads,Serial Speedup,Multithreaded Speedup,Distributed Speedup,...

    # We want to focus on Multithreaded and Distributed for now (ignoring Shared/Task Pool as requested)
    value_vars = ["Multithreaded Speedup", "Distributed Speedup"]

    # Filter only columns that exist
    available_vars = [c for c in value_vars if c in df.columns]

    melted = df.melt(
        id_vars=["Pixel Count", "Kernel Size", "Clusters", "Threads"],
        value_vars=available_vars,
        var_name="Implementation",
        value_name="Speedup",
    )

    melted["Implementation"] = melted["Implementation"].str.replace(" Speedup", "")
    return melted
