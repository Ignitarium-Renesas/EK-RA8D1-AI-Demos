import serial
import struct
import time
import cv2 
import numpy as np

def create_blank_image(width, height, color=(255, 255, 255)):
    """Create a blank white image."""
    return np.full((height, width, 3), color, dtype=np.uint8)

def draw_text(image, text, org, font=cv2.FONT_HERSHEY_SIMPLEX, fontScale=2, color=(255, 0, 0), thickness=2):
    """Draw text on the image."""
    return cv2.putText(image, text, org, font, fontScale, color, thickness, cv2.LINE_AA, False) 

def main():
    baud_rate = 115200  
    serial_port = '/dev/ttyACM1'
    window_name = 'DetectionResult'
    org = (200, 250) 

    # Open the serial port
    ser = serial.Serial(serial_port, baud_rate)

    # Create a blank white image
    width, height = 800, 600
    background_image = create_blank_image(width, height)

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
                    print("Received integer:", integer_value)

                    # Create a new blank image for each iteration
                    background_image = create_blank_image(width, height)
                    integer_value = integer_value - 1
                    # Draw text on the image
                    data = 'HEAD COUNT:' + str(integer_value)
                    background_image = draw_text(background_image, data, org)

                    # Display the image 
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
