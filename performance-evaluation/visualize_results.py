import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

df = pd.read_csv('evaluation_results.csv', index_col=0)

# TODO properly detect outliers
# if library functionality actually gets executed (e.g. global con/destructors) it skews our data
# TODO currently only with trait analysis, library functions are sometimes actually executed
# TODO fix evaluation for those libraries?
df = df[df["timing_without"] <= 0.1]  # remove outliers
df = df[df["timing_with"] <= 0.1]  # remove outliers

df_melt = df.melt(id_vars=["num_libs", "combined_libs_size"],
                  value_vars=["timing_with", "timing_without"],
                  var_name="Timing Type",
                  value_name="Timing")

sns.scatterplot(data=df_melt, x="combined_libs_size", y="Timing", hue="Timing Type")

plt.title("Timing vs. Combined Library Size")
plt.xlabel("Combined Library Size")
plt.ylabel("Timing (seconds)")
plt.savefig("Library_Size.pdf")
plt.cla()

sns.scatterplot(data=df_melt, x="num_libs", y="Timing", hue="Timing Type")

plt.title("Timing vs. number of Libraries")
plt.xlabel("Number of Libraries")
plt.ylabel("Timing (seconds)")
plt.savefig("Library_Count.pdf")
plt.cla()

df["overhaed"] = (df["timing_with"] - df["timing_without"]) / df["timing_without"]

sns.boxplot(data=df, y="overhaed", x=0)
sns.violinplot(data=df, y="overhaed", x=1)
plt.title("Distribution of overhead (as Box and Violin plot)")
plt.ylabel("Overhead")
plt.xticks([])
plt.savefig("overhead_distribution.pdf")
plt.cla()

print(df.describe())
