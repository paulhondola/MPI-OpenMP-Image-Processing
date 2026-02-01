import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
from pathlib import Path

# Set style
sns.set_theme(style="whitegrid")


def _get_dist_threads_col(df: pd.DataFrame) -> str:
    """Identify the column name for distributed threads."""
    return "Threads.1" if "Threads.1" in df.columns else "Threads"


def _melt_speedup_data(df: pd.DataFrame) -> pd.DataFrame:
    """Melts the dataframe to long format for speedup plotting."""
    value_vars = [c for c in df.columns if "Speedup" in c]

    melted = df.melt(
        id_vars=["Pixel Count"],
        value_vars=value_vars,
        var_name="Implementation",
        value_name="Speedup",
    )

    melted["Implementation"] = melted["Implementation"].str.replace(" Speedup", "")
    return melted


def _draw_speedup_graph(ax: plt.Axes, melted_df: pd.DataFrame, k_size: int) -> None:
    """Plots lines on a specific axes using pre-melted data."""
    sns.lineplot(
        data=melted_df,
        x="Pixel Count",
        y="Speedup",
        hue="Implementation",
        marker="o",
        ax=ax,
    )
    ax.legend(loc="upper left")
    ax.set_title(f"Kernel Size = {k_size}")
    ax.set_xscale("log")
    ax.set_xlabel("Pixel Count")
    ax.grid(True)


def _plot_speedup_by_kernel_size(
    subset: pd.DataFrame, cluster: int, thread: int, output_dir: Path
) -> None:
    """Generates and saves the speedup plot for a specific cluster/thread configuration."""
    # We expect Kernel Size 3 and 5 typically
    kernel_sizes = sorted(subset["Kernel Size"].unique())

    if len(kernel_sizes) == 0:
        return

    # Create subplots
    fig, axes = plt.subplots(1, len(kernel_sizes), figsize=(12, 6), sharey=True)

    # Ensure axes is iterable if there's only one kernel size
    if len(kernel_sizes) == 1:
        axes = [axes]

    for ax, k_size in zip(axes, kernel_sizes):
        # 1. Filter by kernel size
        sub_k = subset[subset["Kernel Size"] == k_size].copy()

        # 2. Melt / Aggregate
        melted_data = _melt_speedup_data(sub_k)

        # 3. Plot
        _draw_speedup_graph(ax, melted_data, k_size)

    # Set shared y-label on the first subplot
    axes[0].set_ylabel("Speedup (vs Serial)")

    plt.suptitle(
        f"Speedup vs Image Size\n(Clusters={cluster}, Distributed Threads={thread})"
    )
    plt.tight_layout()

    filename = f"speedup_size_C{cluster}_T{thread}.png"
    plt.savefig(output_dir / filename)
    plt.close()


def plot_speedup_vs_size(df: pd.DataFrame, output_dir: Path) -> None:
    """Speedup vs Image Size (Pixel Count) for specific configurations"""
    print("Plotting Speedup vs Image Size...")

    if "Clusters" not in df.columns:
        print("Error: 'Clusters' column not found in data.")
        return

    dist_threads_col = _get_dist_threads_col(df)

    # Filter for distributed configurations (Clusters > 1)
    distributed_df = df[df["Clusters"] > 1].copy()

    if distributed_df.empty:
        print("No distributed configurations found.")
        return

    # Get unique combinations of Clusters and Distributed Threads
    combinations = (
        distributed_df[["Clusters", dist_threads_col]].drop_duplicates().values
    )

    for cluster, thread in combinations:
        # Filter for this specific configuration
        subset = distributed_df[
            (distributed_df["Clusters"] == cluster)
            & (distributed_df[dist_threads_col] == thread)
        ]
        _plot_speedup_by_kernel_size(subset, cluster, thread, output_dir)


def _prepare_efficiency_data(df: pd.DataFrame) -> tuple[pd.DataFrame, int]:
    """Prepares the melted dataframe for efficiency plotting."""

    # Filter for max image size
    max_pixels = df["Pixel Count"].max()
    subset = df[df["Pixel Count"] == max_pixels].copy()

    # Calculate Total Cores for the row (Clusters * Threads per process)
    subset["Total Cores"] = subset["Clusters"] * subset[_get_dist_threads_col(df)]

    # Filter out single core
    subset = subset[subset["Total Cores"] >= 2]

    if subset.empty:
        return pd.DataFrame(), max_pixels

    # Melt logic to get Efficiency for each implementation
    value_vars = [c for c in df.columns if "Speedup" in c and "Serial" not in c]

    melted_eff = subset.melt(
        id_vars=["Total Cores", "Kernel Size"],
        value_vars=value_vars,
        var_name="Implementation",
        value_name="Speedup",
    )

    melted_eff["Implementation"] = melted_eff["Implementation"].str.replace(
        " Speedup", ""
    )
    melted_eff["Efficiency"] = melted_eff["Speedup"] / melted_eff["Total Cores"]

    return melted_eff, max_pixels


def _draw_efficiency_plot(
    melted_eff: pd.DataFrame, max_pixels: int, output_dir: Path
) -> None:
    """Draws and saves the efficiency plot."""
    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=melted_eff,
        x="Total Cores",
        y="Efficiency",
        hue="Implementation",
        style="Kernel Size",
        markers=True,
    )
    plt.axhline(1.0, color="r", linestyle="--", label="Ideal")
    plt.legend(loc="upper right")
    plt.title(f"Parallel Efficiency vs Total Cores\n(Image Size={max_pixels})")
    plt.ylabel("Efficiency (Speedup / Cores)")
    plt.xlabel("Total Cores")
    plt.xlim(left=1.5)
    plt.ylim(0, 1.2)
    plt.tight_layout()
    plt.savefig(output_dir / "efficiency.png")
    plt.close()


def plot_efficiency(df: pd.DataFrame, output_dir: Path) -> None:
    """Efficiency vs Cores"""
    print("Plotting Efficiency...")

    if "Clusters" not in df.columns:
        print("Error: 'Clusters' column missing for efficiency calculation.")
        return

    melted_eff, max_pixels = _prepare_efficiency_data(df)

    if melted_eff.empty:
        print("No parallel configurations found for efficiency plot.")
        return

    _draw_efficiency_plot(melted_eff, max_pixels, output_dir)
