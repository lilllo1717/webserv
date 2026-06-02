import sys
import os
import datetime
from urllib.parse import parse_qs, unquote_plus

def parse_query_string(qs):
    params = {}
    if not qs:
        return params
    for part in qs.split("&"):
        if "=" in part:
            key, val = part.split("=", 1)
            params[key] = unquote_plus(val)
    return params

def get_current_time(tz_name):
    try:
        import zoneinfo
        tz = zoneinfo.ZoneInfo(tz_name)
        now = datetime.datetime.now(tz)
        return now
    except Exception:
        return datetime.datetime.utcnow()

def time_bar(hour):
    filled = int((hour / 24) * 20)
    bar = "█" * filled + "░" * (20 - filled)
    if 6 <= hour < 9:
        period, mood, bg = "🌅 Dawn",        "The city is waking up.",          "#1a1a0a"
    elif 9 <= hour < 12:
        period, mood, bg = "🌤 Morning",     "Streets are busy with commuters.", "#0a1a1a"
    elif 12 <= hour < 14:
        period, mood, bg = "☀️ Noon",        "Peak activity — lunch hour.",      "#1a1500"
    elif 14 <= hour < 18:
        period, mood, bg = "🌤 Afternoon",   "Afternoon work hours.",            "#0a1a1a"
    elif 18 <= hour < 21:
        period, mood, bg = "🌆 Evening",     "People heading home.",             "#1a0a1a"
    elif 21 <= hour < 24:
        period, mood, bg = "🌙 Night",       "The city winds down.",             "#0a0a1a"
    else:
        period, mood, bg = "🌑 Deep Night",  "Most of the city is asleep.",      "#050510"
    return bar, period, mood, bg

# ── Read CGI environment ──────────────────────────────────────
query_string = os.environ.get("QUERY_STRING", "")
params = parse_query_string(query_string)

city    = params.get("city", "Unknown")
tz_name = params.get("tz",   "UTC")
flag    = params.get("flag",  "🌍")
utc     = params.get("utc",   "UTC+00:00")

# ── Get fresh time for this city ──────────────────────────────
now = get_current_time(tz_name)
time_str   = now.strftime("%H:%M:%S")
date_str   = now.strftime("%A, %d %B %Y")
bar, period, mood, accent_bg = time_bar(now.hour)

# ── Build some fun facts based on hour ───────────────────────
hour = now.hour
if 0 <= hour < 6:
    activity = "🌙 Most residents are asleep"
    tip = "Not a great time to call someone here."
elif 6 <= hour < 9:
    activity = "☕ Morning routines — coffee, commute"
    tip = "Early birds are up. Business opens soon."
elif 9 <= hour < 12:
    activity = "💼 Business hours in full swing"
    tip = "Good time to schedule a call."
elif 12 <= hour < 14:
    activity = "🍽  Lunch break"
    tip = "People may be away from their desks."
elif 14 <= hour < 18:
    activity = "💻 Afternoon work session"
    tip = "Peak productivity hours."
elif 18 <= hour < 21:
    activity = "🏠 Evening — dinner and family time"
    tip = "Avoid work calls unless urgent."
else:
    activity = "🌆 Late evening wind-down"
    tip = "Most offices are closed."

html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>{flag} {city} — City Info</title>
<style>
  * {{ box-sizing: border-box; margin: 0; padding: 0; }}
  body {{
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: #0f0f1a;
    color: #e8e8f0;
    min-height: 100vh;
    padding: 40px 20px;
  }}
  .back {{
    display: inline-block;
    margin-bottom: 30px;
    color: #4a6aff;
    text-decoration: none;
    font-size: 0.9rem;
  }}
  .back:hover {{ color: #a0c4ff; }}
  .hero {{
    max-width: 600px;
    margin: 0 auto;
    background: #1a1a2e;
    border: 1px solid #2a2a4a;
    border-radius: 16px;
    padding: 36px;
  }}
  .hero-header {{
    display: flex;
    align-items: center;
    gap: 16px;
    margin-bottom: 24px;
  }}
  .big-flag {{ font-size: 4rem; }}
  .city-name {{ font-size: 2rem; font-weight: 700; color: #c0c0ff; }}
  .tz-name   {{ font-size: 0.85rem; color: #555; margin-top: 4px; }}
  .time-display {{
    font-size: 3.5rem;
    font-weight: 700;
    color: #ffffff;
    letter-spacing: 2px;
    margin-bottom: 6px;
  }}
  .date-display {{
    font-size: 1rem;
    color: #888;
    margin-bottom: 24px;
  }}
  .divider {{
    border: none;
    border-top: 1px solid #2a2a4a;
    margin: 20px 0;
  }}
  .section-title {{
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 2px;
    color: #555;
    margin-bottom: 10px;
  }}
  .bar-wrap {{
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 6px;
  }}
  .bar {{
    font-size: 0.65rem;
    color: #4a6aff;
    letter-spacing: -1px;
    flex: 1;
  }}
  .period {{ font-size: 0.85rem; color: #aaa; }}
  .mood {{ font-size: 0.85rem; color: #666; margin-bottom: 20px; }}
  .info-row {{
    display: flex;
    justify-content: space-between;
    padding: 10px 0;
    border-bottom: 1px solid #1e1e3a;
    font-size: 0.9rem;
  }}
  .info-label {{ color: #555; }}
  .info-value {{ color: #c0c0ff; }}
  .tip-box {{
    background: #12122a;
    border-left: 3px solid #4a6aff;
    border-radius: 4px;
    padding: 12px 16px;
    margin-top: 20px;
    font-size: 0.85rem;
    color: #888;
  }}
  .utc-badge {{
    display: inline-block;
    background: #252550;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 0.75rem;
    color: #7070cc;
  }}
  footer {{
    text-align: center;
    margin-top: 30px;
    color: #333;
    font-size: 0.75rem;
  }}
  .debug {{
    max-width: 600px;
    margin: 20px auto 0;
    background: #0a0a15;
    border: 1px solid #1a1a3a;
    border-radius: 8px;
    padding: 16px;
    font-family: monospace;
    font-size: 0.75rem;
    color: #444;
  }}
  .debug-title {{ color: #333; margin-bottom: 8px; }}
</style>
</head>
<body>

<div style="max-width:600px; margin:0 auto;">
  <a class="back" href="/cgi-bin/worldclock.py">← Back to World Clock</a>
</div>

<div class="hero">
  <div class="hero-header">
    <span class="big-flag">{flag}</span>
    <div>
      <div class="city-name">{city}</div>
      <div class="tz-name">{tz_name}</div>
    </div>
  </div>

  <div class="time-display">{time_str}</div>
  <div class="date-display">{date_str}</div>

  <hr class="divider">

  <div class="section-title">Time of Day</div>
  <div class="bar-wrap">
    <div class="bar">{bar}</div>
    <div class="period">{period}</div>
  </div>
  <div class="mood">{mood}</div>

  <hr class="divider">

  <div class="section-title">Details</div>
  <div class="info-row">
    <span class="info-label">UTC Offset</span>
    <span class="info-value"><span class="utc-badge">{utc}</span></span>
  </div>
  <div class="info-row">
    <span class="info-label">Current Activity</span>
    <span class="info-value">{activity}</span>
  </div>
  <div class="info-row">
    <span class="info-label">Day of Year</span>
    <span class="info-value">Day {now.timetuple().tm_yday} of 365</span>
  </div>
  <div class="info-row">
    <span class="info-label">Week Number</span>
    <span class="info-value">Week {now.isocalendar()[1]}</span>
  </div>
  <div class="info-row">
    <span class="info-label">Unix Timestamp</span>
    <span class="info-value">{int(now.timestamp())}</span>
  </div>

  <div class="tip-box">💡 {tip}</div>
</div>

<div class="debug">
  <div class="debug-title"># CGI environment (for testing)</div>
  <div>QUERY_STRING = {query_string}</div>
  <div>SCRIPT_NAME  = {os.environ.get('SCRIPT_NAME', 'n/a')}</div>
  <div>REQUEST_METHOD = {os.environ.get('REQUEST_METHOD', 'n/a')}</div>
  <div>SERVER_NAME  = {os.environ.get('SERVER_NAME', 'n/a')}</div>
  <div>SERVER_PORT  = {os.environ.get('SERVER_PORT', 'n/a')}</div>
</div>

<footer>Generated by cityinfo.py &nbsp;·&nbsp; Python CGI &nbsp;·&nbsp; webserv/1.0</footer>
</body>
</html>
"""

body = html.encode("utf-8")
sys.stdout.buffer.write(b"Content-Type: text/html; charset=utf-8\r\n")
sys.stdout.buffer.write(f"Content-Length: {len(body)}\r\n".encode())
sys.stdout.buffer.write(b"\r\n")
sys.stdout.buffer.write(body)
sys.stdout.buffer.flush()
