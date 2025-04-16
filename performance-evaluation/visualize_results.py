from fileinput import filename

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import seaborn as sns
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--input", default='evaluation_results.csv', required=False)

args = parser.parse_args()

df = pd.read_csv(args.input, index_col=0)

# convert to MB
df["combined_libs_size"] = df["combined_libs_size"] / 1e6

df_melt = df.melt(id_vars=["num_libs", "combined_libs_size"],
                  value_vars=["timing_with", "timing_without"],
                  var_name="Timing Type",
                  value_name="Timing")

new_labels = ["With trait check", "No trait check"]

ax = sns.scatterplot(data=df_melt, x="combined_libs_size", y="Timing", hue="Timing Type")

legend = ax.get_legend()
# Rename the legend entries
if legend:
    for i, text in enumerate(legend.get_texts()):
        if i < len(new_labels):
            text.set_text(new_labels[i])
    # Remove the legend title
    legend.set_title(None)

plt.title("Timing vs. Combined Library Size")
plt.xlabel("Combined Library Size (MB)")
plt.ylabel("Timing (seconds)")
plt.savefig("Library_Size.pdf")
# manual setting of xtics


plt.cla()

ax = sns.scatterplot(data=df_melt, x="num_libs", y="Timing", hue="Timing Type")

legend = ax.get_legend()
# Rename the legend entries
if legend:
    for i, text in enumerate(legend.get_texts()):
        if i < len(new_labels):
            text.set_text(new_labels[i])
    # Remove the legend title
    legend.set_title(None)

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
