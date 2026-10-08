#!/usr/bin/env python3
"""IR detector: finds the brightest IR spot, prints it, sends it over UDP.

Packet: camera_id,visible,x,y,area,level
  visible = 1 if a spot was found, 0 otherwise (x, y, area, level are then 0)
  level   = brightness at the spot
"""

import argparse
import socket
import time

import cv2
from picamera2 import Picamera2


def find_spot(img, threshold, min_area, max_area):
    """Find the brightest valid blob in a grayscale image.

    Returns (result, mask), where result is (x, y, area, level) or None,
    and mask is the black/white image the detector actually used.
    """
    # Blur first so single noisy pixels don't count as spots.
    blur = cv2.GaussianBlur(img, (5, 5), 0)

    # Everything at or above the threshold becomes white, the rest black.
    mask = cv2.threshold(blur, threshold, 255, cv2.THRESH_BINARY)[1]

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    contours = [c for c in contours if min_area <= cv2.contourArea(c) <= max_area]
    if not contours:
        return None, mask

    def brightness_then_size(c):
        # Rank blobs by their peak brightness; use area to break ties.
        x, y, w, h = cv2.boundingRect(c)
        return int(blur[y:y + h, x:x + w].max()), cv2.contourArea(c)

    best = max(contours, key=brightness_then_size)

    # The blob's center of mass is the reported coordinate.
    m = cv2.moments(best)
    if m["m00"] == 0:
        return None, mask
    cx, cy = int(m["m10"] / m["m00"]), int(m["m01"] / m["m00"])
    return (cx, cy, cv2.contourArea(best), int(blur[cy, cx])), mask


def parse_args():
    ap = argparse.ArgumentParser(description="AirGraffiti Raspberry Pi IR detector")
    add = ap.add_argument

    add("--width", type=int, default=640)
    add("--height", type=int, default=480)

    # Detection
    add("--threshold", type=int, default=220, help="Minimum brightness (0-255)")
    add("--min-area", type=float, default=40)
    add("--max-area", type=float, default=10000)

    # Camera exposure (lower exposure makes an IR LED stand out from the background)
    add("--exposure", type=int, default=0, help="Manual exposure in us (0 = auto)")
    add("--gain", type=float, default=1.0, help="Gain (only used with --exposure)")

    add("--preview", action="store_true")

    # Network
    add("--windows-ip", default="192.168.1.100")
    add("--udp-port", type=int, default=5005)
    add("--camera-id", type=int, default=0)
    return ap.parse_args()


def main():
    args = parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    dest = (args.windows_ip, args.udp_port)

    def send(result):
        """Send one UDP packet; all zeros means 'nothing detected'."""
        if result:
            x, y, area, level = result
            msg = f"{args.camera_id},1,{x},{y},{area:.1f},{level}"
        else:
            msg = f"{args.camera_id},0,0,0,0,0"
        try:
            sock.sendto(msg.encode("ascii"), dest)
        except OSError:
            pass  # network down or unreachable: keep the detector running

    # ---- Camera setup ----
    cam = Picamera2()
    cam.configure(cam.create_video_configuration(main={"size": (args.width, args.height), "format": "RGB888"}))
    if args.exposure:
        cam.set_controls({"AeEnable": False,  # turn off auto-exposure
                          "ExposureTime": args.exposure,
                          "AnalogueGain": args.gain})
    cam.start()
    time.sleep(1)  # let the camera settle

    print(f"Camera {args.camera_id} -> {args.windows_ip}:{args.udp_port}. Ctrl+C to stop.")

    last_print = 0.0
    was_visible = False

    try:
        while True:
            frame = cam.capture_array()
            # Despite its name, Picamera2's "RGB888" format is stored in BGR order.
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

            result, mask = find_spot(gray, args.threshold, args.min_area, args.max_area)

            send(result)

            # Print at most 5 times per second, plus once when the source is lost.
            now = time.time()
            if result and now - last_print >= 0.2:
                last_print = now
                print(f"IR at x={result[0]}, y={result[1]} "
                      f"(area={result[2]:.0f}px, level={result[3]})")
            elif not result and was_visible:
                print("IR source no longer detected.")
            was_visible = result is not None

            if args.preview:
                if result:
                    cv2.circle(frame, result[:2], 10, (0, 255, 0), 2)  # green ring on the spot
                    cv2.putText(frame, f"area={result[2]:.0f}", (result[0] + 14, result[1] - 14),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1, cv2.LINE_AA)

                cv2.imshow("AirGraffiti IR Camera", frame)

                # Second window: what the detector sees. White = pixels above the threshold
                cv2.imshow("AirGraffiti IR Mask", mask)

                if cv2.waitKey(1) & 0xFF == ord("q"):  # press Q in a window to quit
                    break

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        send(None)  # tell the receiver the IR source is gone
        sock.close()
        cam.stop()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()