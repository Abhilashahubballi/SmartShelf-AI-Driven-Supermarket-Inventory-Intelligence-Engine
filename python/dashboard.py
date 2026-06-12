"""
SmartShelf — AI Visual Dashboard
Generates all charts:
  1. Stock levels by item
  2. Category distribution
  3. Expiry risk heatmap
  4. Demand forecast vs current stock
  5. Restock recommendations
  6. Anomaly detection plot
  7. Weekly sales trend
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.gridspec as gridspec
import os
import warnings
warnings.filterwarnings("ignore")

from ml_engine import run_all

DATA_DIR   = "../data"
OUTPUT_DIR = "../output/reports"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Color palette
COLORS = {
    "green":      "#1a6b3a",
    "light_green":"#4caf7d",
    "yellow":     "#ffc107",
    "red":        "#e53935",
    "blue":       "#1565c0",
    "light_blue": "#42a5f5",
    "gray":       "#757575",
    "bg":         "#f9f9f9",
}

plt.rcParams.update({
    "font.family":   "DejaVu Sans",
    "axes.spines.top":    False,
    "axes.spines.right":  False,
    "figure.facecolor":   COLORS["bg"],
    "axes.facecolor":     "white",
})


def load(filename):
    path = os.path.join(DATA_DIR, filename)
    return pd.read_csv(path) if os.path.exists(path) else None


# ── CHART 1: Stock Levels ─────────────────────────────────────────────────────
def chart_stock_levels(ax, df):
    df_sorted = df.sort_values("quantity")
    colors = [
        COLORS["red"]          if q <= 3 else
        COLORS["yellow"]       if q <= row["low_stock_threshold"] else
        COLORS["light_green"]
        for q, row in zip(df_sorted["quantity"], df_sorted.itertuples())
    ]
    bars = ax.barh(df_sorted["name"], df_sorted["quantity"], color=colors, edgecolor="white", linewidth=0.5)
    ax.axvline(x=0, color=COLORS["gray"], linewidth=0.5)
    ax.set_title("📦 Current Stock Levels", fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel("Quantity (units)")

    # Legend
    patches = [
        mpatches.Patch(color=COLORS["red"],          label="Critical (≤3)"),
        mpatches.Patch(color=COLORS["yellow"],        label="Low Stock"),
        mpatches.Patch(color=COLORS["light_green"],   label="Adequate"),
    ]
    ax.legend(handles=patches, loc="lower right", fontsize=8)

    # Value labels
    for bar, val in zip(bars, df_sorted["quantity"]):
        ax.text(val + 0.3, bar.get_y() + bar.get_height()/2,
                str(val), va="center", fontsize=8, color=COLORS["gray"])


# ── CHART 2: Category Distribution ───────────────────────────────────────────
def chart_category_distribution(ax, df):
    cat_counts = df.groupby("category")["quantity"].sum().sort_values(ascending=False)
    bars = ax.bar(cat_counts.index, cat_counts.values,
                  color=plt.cm.Set2(np.linspace(0, 1, len(cat_counts))),
                  edgecolor="white", linewidth=0.5)
    ax.set_title("🏪 Stock by Category", fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel("Category")
    ax.set_ylabel("Total Units")
    ax.tick_params(axis="x", rotation=30)
    for bar, val in zip(bars, cat_counts.values):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                str(val), ha="center", fontsize=9, color=COLORS["gray"])


# ── CHART 3: Demand Forecast vs Current Stock ─────────────────────────────────
def chart_demand_vs_stock(ax, stock_df, demand_df):
    if demand_df is None:
        ax.text(0.5, 0.5, "Run ML engine first", ha="center", va="center",
                transform=ax.transAxes)
        return

    merged = stock_df.merge(demand_df, left_on="id", right_on="item_id", how="inner")
    merged = merged.sort_values("predicted_demand", ascending=False).head(12)

    x = np.arange(len(merged))
    w = 0.35
    ax.bar(x - w/2, merged["quantity"],          width=w, label="Current Stock",
           color=COLORS["light_blue"],  edgecolor="white")
    ax.bar(x + w/2, merged["predicted_demand"],  width=w, label="Predicted Demand",
           color=COLORS["yellow"],      edgecolor="white")

    ax.set_xticks(x)
    ax.set_xticklabels(merged["name_x"], rotation=35, ha="right", fontsize=8)
    ax.set_title("📈 Demand Forecast vs Current Stock", fontsize=13, fontweight="bold", pad=10)
    ax.set_ylabel("Units")
    ax.legend(fontsize=9)


# ── CHART 4: Expiry Risk ──────────────────────────────────────────────────────
def chart_expiry_risk(ax, expiry_df):
    if expiry_df is None:
        ax.text(0.5, 0.5, "Run ML engine first", ha="center", va="center",
                transform=ax.transAxes)
        return

    risk_map   = {"HIGH": 2, "MEDIUM": 1, "LOW": 0}
    color_map  = {"HIGH": COLORS["red"], "MEDIUM": COLORS["yellow"], "LOW": COLORS["light_green"]}

    expiry_df  = expiry_df.sort_values("days_to_expiry")
    bar_colors = [color_map.get(r, COLORS["gray"]) for r in expiry_df["expiry_risk_label"]]

    bars = ax.barh(expiry_df["name"], expiry_df["days_to_expiry"],
                   color=bar_colors, edgecolor="white", linewidth=0.5)
    ax.set_title("🗓️ Days Until Expiry (Risk Level)", fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel("Days to Expiry")

    patches = [
        mpatches.Patch(color=COLORS["red"],         label="HIGH Risk"),
        mpatches.Patch(color=COLORS["yellow"],       label="MEDIUM Risk"),
        mpatches.Patch(color=COLORS["light_green"],  label="LOW Risk"),
    ]
    ax.legend(handles=patches, loc="lower right", fontsize=8)

    for bar, val in zip(bars, expiry_df["days_to_expiry"]):
        ax.text(val + 0.5, bar.get_y() + bar.get_height()/2,
                f"{val}d", va="center", fontsize=8, color=COLORS["gray"])


# ── CHART 5: Smart Restock Recommendations ───────────────────────────────────
def chart_restock(ax, restock_df):
    if restock_df is None:
        ax.text(0.5, 0.5, "Run ML engine first", ha="center", va="center",
                transform=ax.transAxes)
        return

    restock_df = restock_df[restock_df["smart_restock_qty"] > 0].sort_values(
        "smart_restock_qty", ascending=False).head(12)

    bars = ax.bar(restock_df["name"], restock_df["smart_restock_qty"],
                  color=COLORS["green"], edgecolor="white", linewidth=0.5)
    ax.set_title("🔄 AI Smart Restock Quantities", fontsize=13, fontweight="bold", pad=10)
    ax.set_ylabel("Recommended Order Qty")
    ax.tick_params(axis="x", rotation=35)
    for bar, val in zip(bars, restock_df["smart_restock_qty"]):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.3,
                str(val), ha="center", fontsize=9, color=COLORS["gray"])


# ── CHART 6: Sales History Trend ─────────────────────────────────────────────
def chart_sales_trend(ax, sales_df):
    if sales_df is None:
        ax.text(0.5, 0.5, "No sales history yet", ha="center", va="center",
                transform=ax.transAxes, fontsize=12, color=COLORS["gray"])
        ax.set_title("📊 Weekly Sales Trend", fontsize=13, fontweight="bold", pad=10)
        return

    weekly = sales_df.groupby(["week", "category"])["sales"].sum().reset_index()
    for cat, grp in weekly.groupby("category"):
        ax.plot(grp["week"], grp["sales"], marker="o", linewidth=2, label=cat)

    ax.set_title("📊 Weekly Sales Trend by Category", fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel("Week")
    ax.set_ylabel("Total Units Sold")
    ax.legend(fontsize=8, loc="upper left")
    ax.set_xticks(range(1, 14))


# ── CHART 7: Inventory Value Pie ─────────────────────────────────────────────
def chart_inventory_value(ax, df):
    df["value"] = df["quantity"] * df["price"]
    cat_val = df.groupby("category")["value"].sum().sort_values(ascending=False)
    wedges, texts, autotexts = ax.pie(
        cat_val.values, labels=cat_val.index,
        autopct="%1.1f%%", startangle=90,
        colors=plt.cm.Set2(np.linspace(0, 1, len(cat_val))),
        pctdistance=0.82, wedgeprops=dict(edgecolor="white", linewidth=1.5)
    )
    for at in autotexts: at.set_fontsize(8)
    for t in texts:      t.set_fontsize(9)
    total = cat_val.sum()
    ax.set_title(f"💰 Inventory Value by Category\n(Total: ₹{total:,.0f})",
                 fontsize=13, fontweight="bold", pad=10)


# ── MAIN DASHBOARD ────────────────────────────────────────────────────────────
def generate_dashboard():
    print("\n" + "="*56)
    print("   🤖 SmartShelf — Running AI Models...")
    print("="*56)
    run_all()

    print("\n  🎨 Generating visual dashboard...")

    stock_df   = load("stock.csv")
    demand_df  = load("demand_forecast.csv")
    expiry_df  = load("expiry_predictions.csv")
    restock_df = load("restock_recommendations.csv")
    sales_df   = load("sales_history.csv")

    if stock_df is None:
        print("  ❌ stock.csv not found. Run C++ app first.")
        return

    fig = plt.figure(figsize=(20, 22), facecolor=COLORS["bg"])
    fig.suptitle("🛒 SmartShelf — AI Inventory Intelligence Dashboard",
                 fontsize=18, fontweight="bold", y=0.98, color=COLORS["green"])

    gs = gridspec.GridSpec(4, 2, figure=fig, hspace=0.55, wspace=0.35,
                           left=0.07, right=0.97, top=0.95, bottom=0.03)

    ax1 = fig.add_subplot(gs[0, 0])
    ax2 = fig.add_subplot(gs[0, 1])
    ax3 = fig.add_subplot(gs[1, 0])
    ax4 = fig.add_subplot(gs[1, 1])
    ax5 = fig.add_subplot(gs[2, 0])
    ax6 = fig.add_subplot(gs[2, 1])
    ax7 = fig.add_subplot(gs[3, 0])
    ax8 = fig.add_subplot(gs[3, 1])

    chart_stock_levels(ax1, stock_df)
    chart_category_distribution(ax2, stock_df)
    chart_demand_vs_stock(ax3, stock_df, demand_df)
    chart_expiry_risk(ax4, expiry_df)
    chart_restock(ax5, restock_df)
    chart_sales_trend(ax6, sales_df)
    chart_inventory_value(ax7, stock_df)

    # Summary stats in ax8
    ax8.axis("off")
    low_count      = len(stock_df[stock_df["quantity"] <= stock_df["low_stock_threshold"]])
    critical_count = len(stock_df[stock_df["quantity"] <= 3])
    total_value    = (stock_df["quantity"] * stock_df["price"]).sum()

    summary_text = (
        f"📊  SMARTSHELF SUMMARY\n\n"
        f"  Total SKUs        : {len(stock_df)}\n"
        f"  Inventory Value   : ₹{total_value:,.0f}\n"
        f"  Low Stock Items   : {low_count}\n"
        f"  Critical Items    : {critical_count}\n\n"
    )
    if expiry_df is not None:
        high_exp = len(expiry_df[expiry_df["expiry_risk_label"] == "HIGH"])
        summary_text += f"  High Expiry Risk  : {high_exp} item(s)\n"
    if restock_df is not None:
        needs_restock = len(restock_df[restock_df["smart_restock_qty"] > 0])
        summary_text += f"  Items Need Restock: {needs_restock}\n"

    summary_text += f"\n  🤖 AI Models Active:\n"
    summary_text += f"    • Linear Regression (Demand)\n"
    summary_text += f"    • Decision Tree (Expiry Risk)\n"
    summary_text += f"    • Random Forest (Restock Qty)\n"
    summary_text += f"    • Isolation Forest (Anomaly)\n"

    ax8.text(0.05, 0.95, summary_text, transform=ax8.transAxes,
             fontsize=11, verticalalignment="top",
             fontfamily="monospace",
             bbox=dict(boxstyle="round,pad=0.8", facecolor="white",
                       edgecolor=COLORS["green"], linewidth=2))

    # Save
    dash_path = os.path.join(OUTPUT_DIR, "smartshelf_dashboard.png")
    plt.savefig(dash_path, dpi=150, bbox_inches="tight",
                facecolor=COLORS["bg"])
    print(f"\n  ✅ Dashboard saved → {dash_path}")
    plt.show()
    print("\n  Dashboard generation complete!\n")


if __name__ == "__main__":
    generate_dashboard()
