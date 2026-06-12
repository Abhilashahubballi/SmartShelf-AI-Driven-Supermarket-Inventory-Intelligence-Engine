"""
SmartShelf — Demand Forecast Report
Prints a detailed demand forecast table to console.
"""
import pandas as pd
import os
from ml_engine import train_demand_forecast

def print_forecast():
    train_demand_forecast()

    path = "../data/demand_forecast.csv"
    if not os.path.exists(path):
        print("  ❌ Forecast data not found.")
        return

    df = pd.read_csv(path)
    print("\n" + "="*56)
    print("   📈 SmartShelf — Next Week Demand Forecast")
    print("="*56)
    print(f"  {'Item':<22} {'Category':<14} {'Predicted Demand':>16}")
    print("  " + "-"*52)
    for _, row in df.iterrows():
        print(f"  {row['item_name']:<22} {row['category']:<14} {int(row['predicted_demand']):>16} units")
    print("  " + "-"*52)
    print(f"\n  Total predicted units needed: {df['predicted_demand'].sum():.0f}")
    print("\n  ✅ Report complete.\n")

if __name__ == "__main__":
    print_forecast()
