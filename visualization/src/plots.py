import matplotlib.pyplot as plt
import seaborn as sns

# Set style
sns.set_theme(style="whitegrid")


def plot_speedup_vs_size(df, output_dir):
    """Speedup vs Image Size (Pixel Count) for specific configurations"""
    print("Plotting Speedup vs Image Size...")
    combinations = df[["Clusters", "Threads", "Kernel Size"]].drop_duplicates().values

    for cluster, thread, k_size in combinations:
        subset = df[
            (df["Clusters"] == cluster)
            & (df["Threads"] == thread)
            & (df["Kernel Size"] == k_size)
        ]

        if subset.empty:
            continue

        plt.figure(figsize=(10, 6))
        sns.lineplot(
            data=subset, x="Pixel Count", y="Speedup", hue="Implementation", marker="o"
        )
        plt.title(
            f"Speedup vs Image Size\n(Clusters={cluster}, Threads={thread}, Kernel={k_size})"
        )
        plt.xscale("log")  # Image sizes often spans orders of magnitude
        plt.ylabel("Speedup (vs Serial)")
        plt.xlabel("Pixel Count")
        plt.tight_layout()

        filename = f"speedup_size_C{cluster}_T{thread}_K{k_size}.png"
        plt.savefig(output_dir / filename)
        plt.close()


def plot_strong_scaling_threads(df, output_dir):
    """Speedup vs Threads (Strong Scaling within Node)"""
    print("Plotting Speedup vs Threads...")
    # Fix Clusters=1 for thread scaling usually
    # We want to see how adding threads improves performance for a fixed image size and kernel

    # Selecting a representative large image size for better scaling visibility
    # Get max pixel count
    max_pixels = df["Pixel Count"].max()

    subset = df[(df["Pixel Count"] == max_pixels) & (df["Clusters"] == 1)]

    if subset.empty:
        return

    # For thread scaling, we typically look at Multithreaded implementation
    # But Distributed (on 1 cluster) should also scale similarly

    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=subset,
        x="Threads",
        y="Speedup",
        hue="Implementation",
        style="Kernel Size",
        markers=True,
        dashes=False,
    )
    plt.title(
        f"Strong Scaling: Speedup vs Threads\n(Image Size={max_pixels}, Clusters=1)"
    )
    plt.ylabel("Speedup")
    plt.xlabel("Threads per Node")
    plt.tight_layout()
    plt.savefig(output_dir / "scaling_threads.png")
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
    # Calculate Total Cores = Clusters * Threads
    # Efficiency = Speedup / Total Cores

    df_eff = df.copy()
    df_eff["Total Cores"] = df_eff["Clusters"] * df_eff["Threads"]
    df_eff["Efficiency"] = df_eff["Speedup"] / df_eff["Total Cores"]

    max_pixels = df_eff["Pixel Count"].max()
    subset = df_eff[df_eff["Pixel Count"] == max_pixels]

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
    plt.ylim(0, 1.2)
    plt.tight_layout()
    plt.savefig(output_dir / "efficiency.png")
    plt.close()
