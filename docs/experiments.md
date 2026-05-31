# Experiment Guide (Chapter 4)

## Quick reference

| Experiment | What | When to use | Stop condition |
|------------|------|-------------|----------------|
| **E1** | Charge from USB 5V | Reference benchmark | SOC ≥ 99.5% AND I < 50 mA |
| **E2** | Charge from PV (sunny) | Direct sunlight | SOC ≥ 99.5% OR no PV current 5 min |
| **E3** | Charge from PV (cloudy) | Overcast / shade | Same as E2 |
| **E4** | Discharge to load | After full charge | V_load ≤ 3.0V OR SOC ≤ 1% |
| **E5** | SOC accuracy verification | Full validation cycle | Phase 3 (2nd discharge) complete |
| **E6** | Autonomy test | Validate SOC_min table | SOC ≤ 10% from given start |

## How to run

### Preparation
1. Charge the battery to a known starting SOC (use E1 first if you need a reference).
2. Calibrate SOC: if you just finished a full charge, click "Заряджено (100%)" in the UI.
3. Open the web UI and choose the experiment.

### E1 — USB 5V charge
1. Disconnect any other charge source.
2. Connect the USB cable to the input side of INA3221.
3. Click "E1" in the UI.
4. Wait until experiment stops automatically.
5. Download CSV file from "Експерт даних" section.

### E2 — PV sunny
1. Wait for a clear sunny day (cloud_cover < 30% per local forecast).
2. Place panel in direct sunlight, perpendicular to sun rays.
3. Disconnect USB source.
4. Click "E2".

### E3 — PV cloudy
1. Wait for overcast conditions (cloud_cover > 70%) at the same time of day as E2.
2. Same setup as E2.
3. Click "E3".

### E4 — Discharge
1. Battery should be fully charged.
2. Connect load (USB fan or resistor) to the DC-DC output.
3. Click "E4".
4. The experiment will run for several hours.

### E5 — SOC accuracy verification
This is a **3-phase experiment** that takes the longest:
1. **Phase 1:** discharges battery to 0% — establishes the true zero reference.
2. **Phase 2:** charges to 100% — establishes the true full reference.
3. **Phase 3:** discharges again — algorithm SOC is compared to true SOC.

Click "E5" and let it run. The transitions are automatic.

### E6 — Autonomy
1. Charge battery to the target SOC_min (e.g., for class 3 cloudy → 45%).
2. Calibrate SOC if needed (set the value via "Калібрування SOC").
3. Set the target value in the E6 field and click "E6".
4. The experiment measures how long the load can run before SOC drops to 10%.
5. Repeat for all 5 weather classes: 20%, 30%, 45%, 60%, 70%.

## Data export

Every experiment writes a CSV file to SPIFFS automatically. Download from
**"Експерт даних"** section in the web UI.

CSV columns:

| Column | Description |
|--------|-------------|
| `t_s`  | Time since experiment start (seconds) |
| `V_pv` | Voltage on INA3221 PV channel (V) |
| `I_pv_A` | Current on INA3221 PV channel (A) |
| `V_usb` | Voltage on INA3221 USB channel (V) |
| `I_usb_A` | Current on INA3221 USB channel (A) |
| `V_load` | Voltage on INA219 (V) |
| `I_load_A` | Current on INA219 (A) |
| `V_bat` | Derived battery voltage (V) |
| `SOC_pct` | Estimated state of charge (%) |
| `E_in_Wh` | Cumulative energy in (Wh) |
| `E_out_Wh` | Cumulative energy out (Wh) |

Use Excel/Origin/Python to plot the curves for your thesis chapter.

## ThingSpeak parallel logging

The firmware also publishes data to ThingSpeak every 15 seconds:
- Field 1: V_charge
- Field 2: I_charge (mA)
- Field 3: V_discharge
- Field 4: I_discharge (mA)
- Field 5: SOC (%)
- Field 6: Power (W)

ThingSpeak builds plots automatically. Screenshots of these plots can be used
directly as figures in your thesis.

## Tips

- **Always calibrate SOC** before starting an experiment if the battery state is known.
- **Run each experiment 3 times** for statistical significance (as stated in Section 4.1).
- **Note the weather conditions** for E2/E3 from Open-Meteo: cloud_cover, temperature,
  solar radiation. Save these as supplementary data.
- **Don't change WiFi during an experiment** — it interrupts ThingSpeak logging.
- The **local CSV file** is the primary data source; ThingSpeak is the backup.
