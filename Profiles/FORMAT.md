# Calibration CSV format

The input is a **correction EQ profile**, not raw headphone or room measurements. It must contain exactly six lines (comments beginning `#` are allowed), in ascending order. Frequencies in Hz are fixed; each gain is limited to ±12 dB.

```
60,0
150,0
350,0
1000,0
3500,0
9000,0
```

The example above is **flat / no correction**, not a measured headphone profile. Do not label it as calibrated. To create a genuine correction, obtain reputable measured data for the exact headphone or speaker/listening configuration, choose a target, compute smoothed difference values at these bands, and validate with listening and/or repeat measurements. Raw measurement CSVs are not currently accepted. Headphone fit and unit variation limit accuracy.
