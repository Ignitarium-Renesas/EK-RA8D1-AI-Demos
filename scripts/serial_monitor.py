import serial
import struct
import time
import cv2 
import numpy as np

def create_background_image(width, height, color=(255, 255, 255)):
    # Create a blank white image
    background = np.full((height, width, 3), color, dtype=np.uint8)

    # Load the images
    overlay = cv2.imread('./logos/IGN_LOGO.png', cv2.IMREAD_UNCHANGED)#Change the path as required
    overlay1 = cv2.imread('./logos/renesas_logo1.png', cv2.IMREAD_UNCHANGED)#Change the path as required

    # Resize the images
    overlay = cv2.resize(overlay, (int(overlay.shape[1] * 0.7), int(overlay.shape[0] * 0.6)))
    overlay1 = cv2.resize(overlay1, (int(overlay1.shape[1] * 0.3), int(overlay1.shape[0] * 0.4)))

    # Define the position for overlay1 (top right corner)
    x_offset1 = width - overlay1.shape[1] - 10  # 10 pixels from the right edge
    y_offset1 = 20  # 20 pixels from the top edge

    # Define the position for overlay (bottom right corner)
    x_offset = width - overlay.shape[1] - 10  # 10 pixels from the right edge
    y_offset = height - overlay.shape[0] - 10  # 10 pixels from the bottom edge

    # Overlay the smaller images onto the larger image
    for y in range(overlay.shape[0]):
        for x in range(overlay.shape[1]):
            alpha = overlay[y, x, 3] / 255.0  # Scale alpha to range 0-1
            background[y_offset + y, x_offset + x] = (1 - alpha) * background[y_offset + y, x_offset + x] + alpha * overlay[y, x, :3]

    for y in range(overlay1.shape[0]):
        for x in range(overlay1.shape[1]):
            # Check if pixel is not fully transparent
            if overlay1[y, x, 3] > 0:
                background[y_offset1 + y, x_offset1 + x] = overlay1[y, x, :3]  # Copy BGR channels only

    return background

def draw_text(image, text, org, font=cv2.FONT_HERSHEY_SIMPLEX, fontScale=2, color=(255, 0, 0), thickness=2):
    """Draw text on the image."""
    return cv2.putText(image, text, org, font, fontScale, color, thickness, cv2.LINE_AA, False) 

def draw_box(image, org, text_size, color=(255, 0, 0), thickness=2, padding=10):
    """Draw a box around the text."""
    x, y = org
    x = x - 50
    y = y + 40
    width, height = text_size
    width = width + 80
    height = height + 60
    # Adjust the box dimensions based on text size and padding
    box_width = width + 2 * padding
    box_height = height + 2 * padding
    return cv2.rectangle(image, (x, y), (x + box_width, y - box_height), color, thickness)


def main():
    baud_rate = 115200  
    serial_port = '/dev/ttyACM0'
    window_name = 'DetectionResult'
    org = (165, 300) 

    # Open the serial port
    ser = serial.Serial(serial_port, baud_rate)

    # Create a blank white image
    width, height = 800, 600
    background_image = create_background_image(width, height)

    try:
        while True:
            # Write command to serial port
            ser.write(b'c')

            # Check if data is available
            if ser.in_waiting > 0:
                # Read data from the serial port
                data_bytes = ser.read(4)

                # Unpack bytes into uint32_t integer
                try:
                    integer_value = struct.unpack('<I', data_bytes)[0]
                    # print("Received integer:", integer_value)

                    # Create a new blank image for each iteration
                    background_image = create_background_image(width, height)
                    integer_value = integer_value - 1
                    # Draw text on the image
                    data = 'HEAD COUNT:' + str(integer_value)
                    background_image = draw_text(background_image, data, org)

                    # Get the size of the text
                    text_size = cv2.getTextSize(data, cv2.FONT_HERSHEY_SIMPLEX, 2, 2)[0]

                    # Draw a box around the text
                    background_image = draw_box(background_image, org, text_size)

                    # Display the image 
                    cv2.namedWindow(window_name, cv2.WND_PROP_FULLSCREEN)
                    cv2.setWindowProperty(window_name, cv2.WND_PROP_FULLSCREEN,cv2.WINDOW_FULLSCREEN)
                    cv2.imshow(window_name, background_image)
                    cv2.waitKey(1)

                except struct.error:
                    print("Failed to unpack data bytes")

            # Handle keyboard interruption
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("Exiting program...")
    finally:
        # Close the serial port
        ser.close()
        # Close OpenCV windows
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
