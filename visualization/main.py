import argparse
from pathlib import Path
from src.data import load_data, preprocess_data
from src.plots import (
    plot_speedup_vs_size,
    plot_strong_scaling_threads,
    plot_strong_scaling_clusters,
    plot_efficiency,
)


def main():
    parser = argparse.ArgumentParser(description="Generate performance plots.")
    parser.add_argument(
        "--data",
        type=str,
        default="../data/chrono/speedups_data.csv",
        help="Path to data CSV",
    )
    parser.add_argument(
        "--output",
        type=str,
        default="../data/plots",
        help="Output directory for plots",
    )
    args = parser.parse_args()

    data_path = Path(args.data)
    output_dir = Path(args.output)

    if not output_dir.exists():
        output_dir.mkdir(parents=True)

    print(f"Loading data from {data_path}...")
    df = load_data(data_path)

    print("Preprocessing data...")
    melted_df = preprocess_data(df)

    # Generate Plots
    plot_speedup_vs_size(melted_df, output_dir)
    plot_strong_scaling_threads(melted_df, output_dir)
    plot_strong_scaling_clusters(melted_df, output_dir)
    plot_efficiency(melted_df, output_dir)

    print(f"Done! Plots saved to {output_dir.resolve()}")


if __name__ == "__main__":
    main()
