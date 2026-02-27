import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# Read CSV
log = pd.read_csv('motion_log.csv')

# Plot
fig, ax = plt.subplots(figsize=(12, 5))

# Use milliseconds since start for x axis, starting at zero
x = log['Milliseconds'] - log['Milliseconds'].iloc[0]

ax.clear()
ax.plot(x, log['MotionLevel%'], color='royalblue', linewidth=2, label='Motion Level (%)')

# Format x-axis labels to divide by 10
ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda val, pos: f'{int(val/10)}'))

# Draw average line
mean_level = log['MotionLevel%'].mean()
ax.axhline(mean_level, color='orange', linestyle='--', linewidth=2, label=f'Average ({mean_level:.2f}%)')

# Binomial (moving) fit (Savitzky-Golay filter for smoothing)
try:
    from scipy.signal import savgol_filter
    smooth = savgol_filter(log['MotionLevel%'], window_length=9, polyorder=2)
    ax.plot(x, smooth, color='green', linewidth=2, label='Binomial Fit (Savitzky-Golay)')
except ImportError:
    # Fallback: simple rolling mean
    smooth = log['MotionLevel%'].rolling(window=9, center=True, min_periods=1).mean()
    ax.plot(x, smooth, color='green', linewidth=2, label='Rolling Mean (9)')

# Polynomial (binomial) fit: 2nd or 3rd degree
for degree in [3, 4]:
    coeffs = np.polyfit(x, log['MotionLevel%'], degree)
    poly_fit = np.polyval(coeffs, x)
    ax.plot(x, poly_fit, linestyle='-', linewidth=2, label=f'{degree}nd/rd Degree Fit', color=('purple' if degree==2 else 'darkred'))

# Optional: highlight peaks
peaks = log[log['MotionLevel%'] > log['MotionLevel%'].quantile(0.95)]
ax.scatter(peaks['Milliseconds'] - log['Milliseconds'].iloc[0], peaks['MotionLevel%'], color='crimson', s=30, label='Peaks', zorder=3)

# Labels and title
ax.set_xlabel('Milliseconds since start')
ax.set_ylabel('Motion Level (%)')
ax.set_title('Motion Level Over Time')
ax.grid(True, linestyle='--', alpha=0.4)
ax.legend()
plt.tight_layout()
plt.grid(True, linestyle='--', alpha=0.4)
plt.show()
