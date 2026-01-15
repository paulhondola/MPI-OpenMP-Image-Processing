import pandas as pd
import os


def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.")
        exit(1)
    return pd.read_csv(filepath)


def preprocess_data(df):
    # Rename "Width * Height" to "Pixel Count"
    df = df.rename(columns={"Width * Height": "Pixel Count"})

    # Pandas automatically handles duplicate columns by appending .1, .2, etc.
    # The header is:
    # Width * Height, Kernel Size, Serial Speedup, Threads, Multithreaded Speedup, Clusters, Threads, Distributed Speedup, Shared Speedup, Task Pool Speedup
    # So we likely have: 'Threads' (for Multithreaded) and 'Threads.1' (for Distributed+)

    # Common columns for all
    base_cols = ["Pixel Count", "Kernel Size"]

    parsed_dfs = []

    # 1. Serial (Baseline)
    # Serial usually has Threads=1, Clusters=1 by definition in this context (or N/A)
    # But usually we just plot it as baseline 1.0. The CSV has "Serial Speedup".
    if "Serial Speedup" in df.columns:
        sub = df[base_cols + ["Serial Speedup"]].copy()
        sub["Implementation"] = "Serial"
        sub["Clusters"] = 1
        sub["Threads"] = 1
        sub = sub.rename(columns={"Serial Speedup": "Speedup"})
        parsed_dfs.append(sub)

    # 2. Multithreaded
    # Uses the first 'Threads' column
    if "Multithreaded Speedup" in df.columns and "Threads" in df.columns:
        sub = df[base_cols + ["Threads", "Multithreaded Speedup"]].copy()
        sub["Implementation"] = "Multithreaded"
        sub["Clusters"] = 1
        sub = sub.rename(columns={"Multithreaded Speedup": "Speedup"})
        parsed_dfs.append(sub)

    # 3. Distributed / Shared / Task Pool
    # These use 'Clusters' and the second 'Threads' column (likely 'Threads.1')
    threads_col_2 = "Threads.1" if "Threads.1" in df.columns else "Threads"

    # helper to extract
    def extract_mode(mode_name, col_name):
        if col_name in df.columns and "Clusters" in df.columns:
            # Check if threads_col_2 exists, otherwise fallback to 'Threads' if unique?
            # But based on file structure, it should be there if duplicates existed.
            t_col = threads_col_2 if threads_col_2 in df.columns else "Threads"

            sub = df[base_cols + ["Clusters", t_col, col_name]].copy()
            sub["Implementation"] = mode_name
            sub = sub.rename(columns={col_name: "Speedup", t_col: "Threads"})
            parsed_dfs.append(sub)

    extract_mode("Distributed", "Distributed Speedup")
    extract_mode("Shared", "Shared Speedup")
    extract_mode("Task Pool", "Task Pool Speedup")

    if not parsed_dfs:
        return pd.DataFrame()

    final_df = pd.concat(parsed_dfs, ignore_index=True)

    # Convert "Width * Height" string to integer pixel count
    def parse_resolution(res_str):
        try:
            w, h = map(int, str(res_str).split("*"))
            return w * h
        except ValueError:
            return 0  # Handle potential errors or keep original if needed, but 0 is safer for plotting checks

    final_df["Pixel Count"] = final_df["Pixel Count"].apply(parse_resolution)

    return final_df
