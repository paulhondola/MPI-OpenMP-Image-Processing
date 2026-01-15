import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd

# Set style
sns.set_theme(style="whitegrid")


def plot_speedup_vs_size(df, output_dir):
    """Speedup vs Image Size (Pixel Count) for specific configurations"""
    print("Plotting Speedup vs Image Size...")
    # Group by Cluster and Threads, disregarding Kernel Size for the loop
    combinations = df[["Clusters", "Threads"]].drop_duplicates().values

    for cluster, thread in combinations:
        # Skip Clusters=1 as requested (only multithreaded line)
        if cluster == 1:
            continue

        subset_base = df[(df["Clusters"] == cluster) & (df["Threads"] == thread)]

        # Comparison Logic:
        # If we are plotting a distributed run (Clusters > 1), we want to compare
        # with the equivalent Multithreaded run (Clusters=1, Threads=Total Cores)
        if cluster > 1:
            total_cores = cluster * thread
            multithreaded_subset = df[
                (df["Implementation"] == "Multithreaded")
                & (df["Clusters"] == 1)
                & (df["Threads"] == total_cores)
            ]
            if not multithreaded_subset.empty:
                subset_base = pd.concat([subset_base, multithreaded_subset])

        # Get unique kernel sizes for this configuration
        kernel_sizes = sorted(subset_base["Kernel Size"].unique())

        if len(kernel_sizes) == 0:
            continue

        # Create subplots
        fig, axes = plt.subplots(1, len(kernel_sizes), figsize=(12, 6), sharey=True)

        # Ensure axes is iterable if there's only one kernel size
        if len(kernel_sizes) == 1:
            axes = [axes]

        for ax, k_size in zip(axes, kernel_sizes):
            subset = subset_base[subset_base["Kernel Size"] == k_size]

            sns.lineplot(
                data=subset,
                x="Pixel Count",
                y="Speedup",
                hue="Implementation",
                marker="o",
                ax=ax,
            )

            ax.set_title(f"Kernel Size = {k_size}")
            ax.set_xscale("log")  # Image sizes often spans orders of magnitude
            ax.set_xlabel("Pixel Count")
            ax.grid(True)

        # Set shared y-label on the first subplot
        axes[0].set_ylabel("Speedup (vs Serial)")

        plt.suptitle(f"Speedup vs Image Size\n(Clusters={cluster}, Threads={thread})")
        plt.tight_layout()

        filename = f"speedup_size_C{cluster}_T{thread}.png"
        plt.savefig(output_dir / filename)
        plt.close()


def plot_strong_scaling_clusters(df, output_dir):
    """Speedup vs Clusters (Strong Scaling across Nodes)"""
    print("Plotting Speedup vs Clusters...")

    max_pixels = df["Pixel Count"].max()
    # Fix Threads to something constant, e.g., max threads or 1
    # Let's pick the max threads available in data
    max_threads = df["Threads"].max()

    subset = df[
        (df["Pixel Count"] == max_pixels)
        & (df["Threads"] == max_threads)
        & (
            df["Implementation"] == "Distributed"
        )  # Only Distributed scales with clusters
    ]

    if subset.empty:
        return

    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=subset,
        x="Clusters",
        y="Speedup",
        hue="Kernel Size",
        marker="o",
        palette="viridis",
    )
    plt.title(
        f"Strong Scaling: Speedup vs Clusters\n(Image Size={max_pixels}, Threads={max_threads})"
    )
    plt.ylabel("Speedup")
    plt.xlabel("Number of Clusters (Nodes)")
    # Force integer ticks for clusters
    plt.xticks(sorted(subset["Clusters"].unique()))
    plt.tight_layout()
    plt.savefig(output_dir / "scaling_clusters.png")
    plt.close()


def plot_efficiency(df, output_dir):
    """Efficiency vs Cores"""
    print("Plotting Efficiency...")

    df_eff = df.copy()
    df_eff["Total Cores"] = df_eff["Clusters"] * df_eff["Threads"]
    df_eff["Efficiency"] = df_eff["Speedup"] / df_eff["Total Cores"]

    max_pixels = df_eff["Pixel Count"].max()
    subset = df_eff[df_eff["Pixel Count"] == max_pixels]

    # Filter out single core
    subset = subset[subset["Total Cores"] >= 2]

    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=subset,
        x="Total Cores",
        y="Efficiency",
        hue="Implementation",
        style="Kernel Size",
        markers=True,
    )
    plt.axhline(1.0, color="r", linestyle="--", label="Ideal")
    plt.title(f"Parallel Efficiency vs Total Cores\n(Image Size={max_pixels})")
    plt.ylabel("Efficiency (Speedup / Cores)")
    plt.xlabel("Total Cores")
    plt.xlim(left=1.5)
    plt.ylim(0, 1.2)
    plt.tight_layout()
    plt.savefig(output_dir / "efficiency.png")
    plt.close()
