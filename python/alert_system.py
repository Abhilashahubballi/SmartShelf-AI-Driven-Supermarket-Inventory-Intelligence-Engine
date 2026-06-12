"""
SmartShelf — Vendor Alert System
Sends:
  1. Email (real — via Gmail SMTP)
  2. SMS (simulated — production-ready format)
  3. AI Voice message (real .mp3 via gTTS)
  4. Scheduled reminders (T-3 days, T-1 day)

Usage:
  python3 alert_system.py <order_id> <delivery_date>
"""

import sys
import os
import json
import smtplib
import csv
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders
from datetime import datetime, timedelta
import schedule
import time
import threading

# ── CONFIG — update these before running ─────────────────────────────────────
EMAIL_CONFIG = {
    "sender_email":    "your_gmail@gmail.com",       # YOUR Gmail
    "sender_password": "your_app_password",           # Gmail App Password
    "vendor_email":    "vendor@example.com",          # Vendor email
    "vendor_name":     "Ravi Supplies",
    "vendor_phone":    "+91 9876543210",
    "store_name":      "SmartShelf Supermarket",
    "store_address":   "MG Road, Bengaluru - 560001",
    "store_phone":     "+91 9123456789",
}

DATA_DIR   = "../data"
OUTPUT_DIR = "../output"


# ── Load order from CSV ───────────────────────────────────────────────────────
def load_order(order_id):
    fname = os.path.join(OUTPUT_DIR, f"vendor_order_{order_id}.csv")
    if not os.path.exists(fname):
        print(f"  ❌ Order file not found: {fname}")
        return None
    items = []
    with open(fname) as f:
        reader = csv.DictReader(f)
        for row in reader:
            items.append(row)
    return items


# ── 1. EMAIL ──────────────────────────────────────────────────────────────────
def send_email(order_id, delivery_date, items, reminder=False):
    cfg = EMAIL_CONFIG
    subject = (f"[SmartShelf] {'REMINDER: ' if reminder else ''}Purchase Order #{order_id} — "
               f"Delivery by {delivery_date}")

    # Build HTML email body
    rows_html = ""
    total = 0.0
    for item in items:
        line_total = float(item.get('total', 0))
        total += line_total
        rows_html += f"""
        <tr>
          <td style="padding:8px;border:1px solid #ddd;">{item['item_name']}</td>
          <td style="padding:8px;border:1px solid #ddd;text-align:center;">{item['category']}</td>
          <td style="padding:8px;border:1px solid #ddd;text-align:center;">{item['ordered_qty']}</td>
          <td style="padding:8px;border:1px solid #ddd;text-align:right;">₹{float(item['unit_price']):.2f}</td>
          <td style="padding:8px;border:1px solid #ddd;text-align:right;">₹{line_total:.2f}</td>
        </tr>"""

    html_body = f"""
    <html><body style="font-family:Arial,sans-serif;color:#333;">
      <div style="background:#1a6b3a;padding:20px;color:white;border-radius:8px 8px 0 0;">
        <h2 style="margin:0;">🛒 {cfg['store_name']}</h2>
        <p style="margin:5px 0 0;">{cfg['store_address']} | {cfg['store_phone']}</p>
      </div>
      <div style="padding:20px;border:1px solid #ddd;border-top:none;border-radius:0 0 8px 8px;">
        <h3>{'⏰ Delivery Reminder — ' if reminder else ''}Purchase Order #{order_id}</h3>
        <p>Dear <b>{cfg['vendor_name']}</b>,<br><br>
        {'This is a reminder that the following order is due for delivery on ' if reminder else
         'Please find below the purchase order. Kindly ensure delivery by '}<b>{delivery_date}</b>.</p>

        <table style="width:100%;border-collapse:collapse;margin-top:15px;">
          <thead>
            <tr style="background:#1a6b3a;color:white;">
              <th style="padding:10px;text-align:left;">Item</th>
              <th style="padding:10px;">Category</th>
              <th style="padding:10px;">Qty</th>
              <th style="padding:10px;">Unit Price</th>
              <th style="padding:10px;">Total</th>
            </tr>
          </thead>
          <tbody>{rows_html}</tbody>
          <tfoot>
            <tr style="background:#f5f5f5;font-weight:bold;">
              <td colspan="4" style="padding:10px;text-align:right;">Total Order Value:</td>
              <td style="padding:10px;text-align:right;">₹{total:.2f}</td>
            </tr>
          </tfoot>
        </table>

        <div style="margin-top:20px;padding:15px;background:#fff8e1;border-left:4px solid #ffc107;border-radius:4px;">
          <b>📦 Delivery Details:</b><br>
          Deliver to: {cfg['store_name']}, {cfg['store_address']}<br>
          Required by: <b>{delivery_date}</b><br>
          Contact: {cfg['store_phone']}
        </div>

        <p style="margin-top:20px;color:#666;font-size:12px;">
          This is an automated message from SmartShelf Inventory Intelligence System.<br>
          Please reply to confirm receipt of this order.
        </p>
      </div>
    </body></html>
    """

    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"]    = cfg["sender_email"]
    msg["To"]      = cfg["vendor_email"]
    msg.attach(MIMEText(html_body, "html"))

    # Attach order CSV
    csv_path = os.path.join(OUTPUT_DIR, f"vendor_order_{order_id}.csv")
    if os.path.exists(csv_path):
        with open(csv_path, "rb") as f:
            part = MIMEBase("application", "octet-stream")
            part.set_payload(f.read())
            encoders.encode_base64(part)
            part.add_header("Content-Disposition",
                            f"attachment; filename=order_{order_id}.csv")
            msg.attach(part)

    try:
        with smtplib.SMTP_SSL("smtp.gmail.com", 465) as server:
            server.login(cfg["sender_email"], cfg["sender_password"])
            server.sendmail(cfg["sender_email"], cfg["vendor_email"], msg.as_string())
        print(f"  ✅ Email sent to {cfg['vendor_email']}")
    except Exception as e:
        print(f"  ⚠️  Email not sent (configure EMAIL_CONFIG): {e}")
        print(f"  📧 [SIMULATED EMAIL PREVIEW]")
        print(f"     To:      {cfg['vendor_email']}")
        print(f"     Subject: {subject}")
        print(f"     Body:    HTML email with order table (₹{total:.2f} total)")


# ── 2. SIMULATED SMS ──────────────────────────────────────────────────────────
def send_sms(order_id, delivery_date, items, reminder=False, days_left=None):
    cfg = EMAIL_CONFIG
    item_count = len(items)
    total = sum(float(i.get('total', 0)) for i in items)

    if reminder:
        sms_text = (
            f"[SmartShelf] REMINDER: Order #{order_id} delivery due in "
            f"{days_left} day(s) ({delivery_date}). "
            f"{item_count} items, ₹{total:.0f}. "
            f"Contact: {cfg['store_phone']}"
        )
    else:
        sms_text = (
            f"[SmartShelf] New Purchase Order #{order_id} received. "
            f"{item_count} items, ₹{total:.0f}. "
            f"Deliver by: {delivery_date}. "
            f"Reply YES to confirm. {cfg['store_phone']}"
        )

    print(f"\n  📱 SMS {'REMINDER ' if reminder else ''}→ {cfg['vendor_phone']}")
    print(f"  ┌{'─'*54}┐")
    # Word wrap
    words = sms_text.split()
    line = "  │ "
    for word in words:
        if len(line) + len(word) + 1 > 57:
            print(line + " " * (57 - len(line)) + "│")
            line = "  │ " + word + " "
        else:
            line += word + " "
    print(line + " " * (57 - len(line)) + "│")
    print(f"  └{'─'*54}┘")
    print(f"  ℹ️  (SMS simulated — integrate Fast2SMS/Twilio for production)")


# ── 3. AI VOICE MESSAGE ───────────────────────────────────────────────────────
def generate_voice_message(order_id, delivery_date, items):
    cfg = EMAIL_CONFIG
    item_count = len(items)
    total = sum(float(i.get('total', 0)) for i in items)

    # Build item list text
    item_texts = []
    for item in items[:5]:  # limit to 5 for voice
        item_texts.append(f"{item['item_name']}, {item['ordered_qty']} units")
    items_spoken = ". ".join(item_texts)
    if len(items) > 5:
        items_spoken += f". And {len(items) - 5} more items"

    script = (
        f"Hello, this is an automated message from {cfg['store_name']}, "
        f"located at {cfg['store_address']}. "
        f"You have received a new purchase order, number {order_id}. "
        f"Items ordered include: {items_spoken}. "
        f"Total order value is rupees {total:.0f}. "
        f"Please ensure delivery by {delivery_date}. "
        f"For any queries, please contact us at {cfg['store_phone']}. "
        f"Thank you and have a great day."
    )

    print(f"\n  🔊 AI Voice Message Script:")
    print(f"  ┌{'─'*54}┐")
    words = script.split()
    line = "  │ "
    for word in words:
        if len(line) + len(word) + 1 > 57:
            print(line + " " * (57 - len(line)) + "│")
            line = "  │ " + word + " "
        else:
            line += word + " "
    print(line + " " * (57 - len(line)) + "│")
    print(f"  └{'─'*54}┘")

    # Try gTTS
    try:
        from gtts import gTTS
        tts = gTTS(text=script, lang='en', slow=False)
        voice_path = os.path.join(OUTPUT_DIR, f"voice_alert_order_{order_id}.mp3")
        tts.save(voice_path)
        print(f"\n  ✅ AI Voice .mp3 generated → {voice_path}")

        # Try to play it
        import platform
        if platform.system() == "Linux":
            os.system(f"mpg123 {voice_path} 2>/dev/null || "
                      f"ffplay -nodisp -autoexit {voice_path} 2>/dev/null || "
                      f"echo '  🔊 Voice file saved. Open {voice_path} to listen.'")
        elif platform.system() == "Darwin":
            os.system(f"afplay {voice_path}")
        elif platform.system() == "Windows":
            os.system(f"start {voice_path}")
    except ImportError:
        print(f"\n  ℹ️  gTTS not installed. Run: pip install gtts")
        print(f"     Script saved for voice generation.")
        # Save script as text
        with open(os.path.join(OUTPUT_DIR, f"voice_script_order_{order_id}.txt"), "w") as f:
            f.write(script)


# ── 4. SCHEDULER — reminder emails/SMS ───────────────────────────────────────
def schedule_reminders(order_id, delivery_date_str, items):
    try:
        delivery = datetime.strptime(delivery_date_str, "%Y-%m-%d")
        today    = datetime.today().replace(hour=0, minute=0, second=0, microsecond=0)

        remind_3 = delivery - timedelta(days=3)
        remind_1 = delivery - timedelta(days=1)

        reminders = []
        if remind_3 >= today:
            reminders.append((remind_3, 3))
        if remind_1 >= today:
            reminders.append((remind_1, 1))
        if delivery >= today:
            reminders.append((delivery, 0))

        if not reminders:
            print("  ℹ️  Delivery date is in the past — no reminders scheduled.")
            return

        print(f"\n  📅 Reminder Schedule for Order #{order_id}:")
        for remind_date, days_left in reminders:
            if days_left == 0:
                label = "On delivery date"
            else:
                label = f"T-{days_left} days"
            print(f"     {label}: {remind_date.strftime('%Y-%m-%d')} → SMS + Email reminder")

        # Save schedule to file (for cron/scheduler to pick up)
        schedule_data = {
            "order_id":      order_id,
            "delivery_date": delivery_date_str,
            "reminders": [
                {"date": r[0].strftime("%Y-%m-%d"), "days_left": r[1]}
                for r in reminders
            ]
        }
        sched_path = os.path.join(OUTPUT_DIR, f"reminder_schedule_{order_id}.json")
        with open(sched_path, "w") as f:
            json.dump(schedule_data, f, indent=2)
        print(f"  📄 Reminder schedule saved → {sched_path}")

    except Exception as e:
        print(f"  ⚠️  Scheduler error: {e}")


# ── MAIN ──────────────────────────────────────────────────────────────────────
def send_all_alerts(order_id, delivery_date):
    print(f"\n{'='*56}")
    print(f"   📣 SmartShelf Vendor Alert System")
    print(f"   Order #{order_id} | Delivery: {delivery_date}")
    print(f"{'='*56}")

    items = load_order(order_id)
    if not items:
        print("  ❌ Could not load order. Check order ID.")
        return

    print(f"\n  Order has {len(items)} item(s).")

    # 1. Email
    send_email(order_id, delivery_date, items)

    # 2. SMS
    send_sms(order_id, delivery_date, items)

    # 3. AI Voice
    generate_voice_message(order_id, delivery_date, items)

    # 4. Schedule reminders
    schedule_reminders(order_id, delivery_date, items)

    print(f"\n{'='*56}")
    print(f"   ✅ All alerts processed for Order #{order_id}")
    print(f"{'='*56}\n")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 alert_system.py <order_id> <delivery_date YYYY-MM-DD>")
        sys.exit(1)
    order_id      = int(sys.argv[1])
    delivery_date = sys.argv[2]
    send_all_alerts(order_id, delivery_date)
