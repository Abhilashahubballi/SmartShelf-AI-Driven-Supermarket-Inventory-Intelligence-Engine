"""
SmartShelf ML Engine
Trains and runs all ML models:
  1. Demand Forecasting       — Linear Regression
  2. Expiry Risk Prediction   — Decision Tree
  3. Smart Restock Quantity   — Random Forest
  4. Customer Segmentation    — K-Means
"""

import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestRegressor, IsolationForest
from sklearn.cluster import KMeans
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_absolute_error, classification_report
import joblib
import os
import warnings
warnings.filterwarnings("ignore")

DATA_DIR   = "../data"
MODEL_DIR  = "../output/models"
os.makedirs(MODEL_DIR, exist_ok=True)


# ── Helper: load stock ────────────────────────────────────────────────────────
def load_stock():
    path = os.path.join(DATA_DIR, "stock.csv")
    if not os.path.exists(path):
        print("  ⚠️  stock.csv not found. Run the C++ app first.")
        return None
    return pd.read_csv(path)


# ── 1. DEMAND FORECASTING ─────────────────────────────────────────────────────
def train_demand_forecast():
    """
    Uses synthetic weekly sales history derived from stock data.
    Predicts how many units of each item will be needed next week.
    """
    print("\n  📈 Training Demand Forecast Model (Linear Regression)...")

    df = load_stock()
    if df is None:
        return None

    # Simulate 12 weeks of sales history per item
    np.random.seed(42)
    records = []
    for _, row in df.iterrows():
        base_demand = max(5, int(row['reorder_qty'] * 0.6))
        for week in range(1, 13):
            seasonal = 1 + 0.2 * np.sin(week * np.pi / 6)
            noise = np.random.normal(0, base_demand * 0.1)
            sales = max(0, int(base_demand * seasonal + noise))
            records.append({
                'item_id':   row['id'],
                'item_name': row['name'],
                'category':  row['category'],
                'week':      week,
                'price':     row['price'],
                'sales':     sales
            })

    sales_df = pd.DataFrame(records)
    sales_df.to_csv(os.path.join(DATA_DIR, "sales_history.csv"), index=False)

    # Features
    le = LabelEncoder()
    sales_df['category_enc'] = le.fit_transform(sales_df['category'])
    X = sales_df[['item_id', 'week', 'price', 'category_enc']]
    y = sales_df['sales']

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    model = LinearRegression()
    model.fit(X_train, y_train)

    mae = mean_absolute_error(y_test, model.predict(X_test))
    print(f"  ✅ Demand model trained | MAE: {mae:.2f} units")

    joblib.dump(model, os.path.join(MODEL_DIR, "demand_model.pkl"))
    joblib.dump(le,    os.path.join(MODEL_DIR, "demand_le.pkl"))

    # Predict next week demand for each item
    latest = sales_df.groupby('item_id').last().reset_index()
    latest['week'] = 13
    X_next = latest[['item_id', 'week', 'price', 'category_enc']]
    latest['predicted_demand'] = model.predict(X_next).astype(int).clip(min=0)
    latest['predicted_demand'] = latest['predicted_demand'].apply(lambda x: max(1, x))

    result = latest[['item_id', 'item_name', 'category', 'predicted_demand']].copy()
    result.to_csv(os.path.join(DATA_DIR, "demand_forecast.csv"), index=False)
    print(f"  📄 Forecast saved → data/demand_forecast.csv")
    return model


# ── 2. EXPIRY RISK PREDICTION ─────────────────────────────────────────────────
def train_expiry_predictor():
    """
    Predicts expiry risk level: LOW / MEDIUM / HIGH
    Based on days until expiry, quantity, and category.
    """
    print("\n  🗓️  Training Expiry Risk Model (Decision Tree)...")

    df = load_stock()
    if df is None:
        return None

    from datetime import datetime
    today = datetime.today()

    def days_to_expiry(expiry_str):
        try:
            exp = datetime.strptime(expiry_str, "%Y-%m-%d")
            return max(0, (exp - today).days)
        except:
            return 365

    df['days_to_expiry'] = df['expiry_date'].apply(days_to_expiry)

    def risk_label(row):
        if row['days_to_expiry'] <= 3:   return 2   # HIGH
        elif row['days_to_expiry'] <= 10: return 1   # MEDIUM
        else:                             return 0   # LOW

    df['risk'] = df.apply(risk_label, axis=1)

    # Augment with synthetic data for training
    np.random.seed(0)
    records = []
    for _ in range(500):
        days  = np.random.randint(0, 60)
        qty   = np.random.randint(1, 100)
        price = np.random.uniform(10, 300)
        if days <= 3:   risk = 2
        elif days <= 10: risk = 1
        else:           risk = 0
        records.append({'days_to_expiry': days, 'quantity': qty, 'price': price, 'risk': risk})

    aug_df = pd.DataFrame(records)
    le = LabelEncoder()
    df['category_enc'] = le.fit_transform(df['category'])

    X = aug_df[['days_to_expiry', 'quantity', 'price']]
    y = aug_df['risk']

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    model = DecisionTreeClassifier(max_depth=5, random_state=42)
    model.fit(X_train, y_train)

    acc = model.score(X_test, y_test)
    print(f"  ✅ Expiry model trained | Accuracy: {acc*100:.1f}%")

    joblib.dump(model, os.path.join(MODEL_DIR, "expiry_model.pkl"))

    # Predict for current stock
    df['category_enc'] = le.fit_transform(df['category'])
    X_stock = df[['days_to_expiry', 'quantity', 'price']]
    df['expiry_risk'] = model.predict(X_stock)
    df['expiry_risk_label'] = df['expiry_risk'].map({0: 'LOW', 1: 'MEDIUM', 2: 'HIGH'})

    result = df[['id', 'name', 'category', 'quantity', 'expiry_date',
                 'days_to_expiry', 'expiry_risk_label']].copy()
    result.to_csv(os.path.join(DATA_DIR, "expiry_predictions.csv"), index=False)
    print(f"  📄 Expiry predictions saved → data/expiry_predictions.csv")
    return model


# ── 3. SMART RESTOCK QUANTITY ─────────────────────────────────────────────────
def train_restock_advisor():
    """
    Random Forest predicts optimal reorder quantity
    based on current stock, demand forecast, price, category.
    """
    print("\n  🔄 Training Restock Advisor (Random Forest)...")

    df = load_stock()
    if df is None:
        return None

    # Load demand forecast if available
    demand_path = os.path.join(DATA_DIR, "demand_forecast.csv")
    if os.path.exists(demand_path):
        demand_df = pd.read_csv(demand_path)
        df = df.merge(demand_df[['item_id', 'predicted_demand']],
                      left_on='id', right_on='item_id', how='left')
        df['predicted_demand'] = df['predicted_demand'].fillna(df['reorder_qty'])
    else:
        df['predicted_demand'] = df['reorder_qty']

    # Optimal restock = predicted_demand + buffer - current_stock
    df['optimal_restock'] = (df['predicted_demand'] * 1.3 - df['quantity']).clip(lower=0).astype(int)

    np.random.seed(7)
    records = []
    for _ in range(600):
        qty       = np.random.randint(0, 50)
        threshold = np.random.randint(5, 20)
        demand    = np.random.randint(10, 80)
        price     = np.random.uniform(10, 300)
        optimal   = max(0, int(demand * 1.3 - qty))
        records.append({'quantity': qty, 'low_stock_threshold': threshold,
                        'predicted_demand': demand, 'price': price,
                        'optimal_restock': optimal})

    aug_df = pd.DataFrame(records)
    X = aug_df[['quantity', 'low_stock_threshold', 'predicted_demand', 'price']]
    y = aug_df['optimal_restock']

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    model = RandomForestRegressor(n_estimators=100, random_state=42)
    model.fit(X_train, y_train)

    mae = mean_absolute_error(y_test, model.predict(X_test))
    print(f"  ✅ Restock model trained | MAE: {mae:.1f} units")

    joblib.dump(model, os.path.join(MODEL_DIR, "restock_model.pkl"))

    # Predict for current stock
    X_stock = df[['quantity', 'low_stock_threshold', 'predicted_demand', 'price']]
    df['smart_restock_qty'] = model.predict(X_stock).astype(int).clip(min=0)

    result = df[['id', 'name', 'category', 'quantity',
                 'low_stock_threshold', 'predicted_demand', 'smart_restock_qty', 'supplier']].copy()
    result.to_csv(os.path.join(DATA_DIR, "restock_recommendations.csv"), index=False)
    print(f"  📄 Restock recommendations saved → data/restock_recommendations.csv")
    return model


# ── 4. ANOMALY DETECTION ──────────────────────────────────────────────────────
def train_anomaly_detector():
    """
    Isolation Forest detects anomalous stock levels
    (sudden drops, unusual quantities).
    """
    print("\n  🔍 Training Anomaly Detector (Isolation Forest)...")

    df = load_stock()
    if df is None:
        return None

    le = LabelEncoder()
    df['category_enc'] = le.fit_transform(df['category'])
    X = df[['quantity', 'price', 'low_stock_threshold', 'reorder_qty', 'category_enc']]

    model = IsolationForest(contamination=0.1, random_state=42)
    df['anomaly'] = model.fit_predict(X)
    df['anomaly_label'] = df['anomaly'].map({1: 'NORMAL', -1: 'ANOMALY'})

    anomalies = df[df['anomaly'] == -1][['id', 'name', 'category', 'quantity', 'anomaly_label']]
    if not anomalies.empty:
        print(f"  ⚠️  {len(anomalies)} anomalous stock level(s) detected:")
        for _, row in anomalies.iterrows():
            print(f"     → {row['name']} (Qty: {row['quantity']})")
    else:
        print("  ✅ No stock anomalies detected.")

    joblib.dump(model, os.path.join(MODEL_DIR, "anomaly_model.pkl"))
    df[['id', 'name', 'category', 'quantity', 'anomaly_label']].to_csv(
        os.path.join(DATA_DIR, "anomaly_report.csv"), index=False)
    print(f"  📄 Anomaly report saved → data/anomaly_report.csv")
    return model


# ── RUN ALL ───────────────────────────────────────────────────────────────────
def run_all():
    print("\n" + "="*56)
    print("   🤖 SmartShelf ML Engine — Training All Models")
    print("="*56)
    train_demand_forecast()
    train_expiry_predictor()
    train_restock_advisor()
    train_anomaly_detector()
    print("\n" + "="*56)
    print("   ✅ All models trained and saved to output/models/")
    print("="*56 + "\n")


if __name__ == "__main__":
    run_all()
