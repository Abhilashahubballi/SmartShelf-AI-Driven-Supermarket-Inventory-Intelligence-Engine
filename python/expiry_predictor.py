"""
SmartShelf — Expiry Risk Report
Shows which items are at HIGH/MEDIUM/LOW risk of expiry.
"""
import pandas as pd
import os
from ml_engine import train_expiry_predictor

def print_expiry_report():
    train_expiry_predictor()

    path = "../data/expiry_predictions.csv"
    if not os.path.exists(path):
        print("  ❌ Expiry predictions not found.")
        return

    df = pd.read_csv(path)
    print("\n" + "="*64)
    print("   🗓️  SmartShelf — Expiry Risk Prediction Report")
    print("="*64)

    for risk, label, emoji in [(2, 'HIGH', '🔴'), (1, 'MEDIUM', '🟡'), (0, 'LOW', '🟢')]:
        subset = df[df['expiry_risk_label'] == ['LOW','MEDIUM','HIGH'][risk]]
        if subset.empty:
            continue
        print(f"\n  {emoji} {label} RISK ({len(subset)} items):")
        print(f"  {'Item':<22} {'Category':<14} {'Qty':>5} {'Expiry':<12} {'Days Left':>9}")
        print("  " + "-"*62)
        for _, row in subset.iterrows():
            print(f"  {row['name']:<22} {row['category']:<14} "
                  f"{int(row['quantity']):>5} {row['expiry_date']:<12} "
                  f"{int(row['days_to_expiry']):>9}d")

    high_risk = df[df['expiry_risk_label'] == 'HIGH']
    if not high_risk.empty:
        print(f"\n  ⚠️  ACTION REQUIRED: {len(high_risk)} item(s) expiring very soon!")
        print("     Consider discount promotions to reduce wastage.")
    print()

if __name__ == "__main__":
    print_expiry_report()
