---
title: "Help: airgapped signing issues"
nav_title: Airgapped signing issues
category: troubleshooting
---

Main article: [Offline transaction signing](offline-tx-signing)

### The QR code won't scan — too bright or too dark

Try these in roughly this order:

- Turn on **Manual exposure** if your webcam supports it and move the slider until the code is crisp
- Make the window bigger so the code is physically larger
- Change your monitor's brightness
- Switch to dark mode under **Settings → Appearance → Theme**
- Vary the distance between camera and screen
- Add light to the room
- Open the settings icon beside the QR code, then lower the fragment length and slow the animation down by raising the ms per fragment
- Failing all of that, try another webcam

### The progress bar sits at 99%

It isn't stuck — a fragment was missed and the scanner is waiting for that part to cycle round again. Keep the code in frame for a few more seconds.

### The webcam is found but the picture is black

Please [report](report-an-issue) it.

On Windows 10, note that 22H2 or newer is required.

### The webcam is connected but isn't listed

Press **Refresh**. If it still doesn't appear, the camera probably needs drivers installed.

### "Double spend" errors when broadcasting

Run **Wallet → Advanced → Rescan spent**, then try again.
