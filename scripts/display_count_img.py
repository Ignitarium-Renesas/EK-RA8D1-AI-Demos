import serial
import struct
import time
import cv2 
import numpy as np
import sys

def create_background_image(width, height, bgr_img = None, color=(255, 255, 255)):
    # Create a blank white image
    background = np.full((height, width, 3), color, dtype=np.uint8)

    # Load the images
    overlay = cv2.imread('./IGN_LOGO.png', cv2.IMREAD_UNCHANGED)#Change the path as required
    overlay1 = cv2.imread('./renesas_logo1.png', cv2.IMREAD_UNCHANGED)#Change the path as required

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

    # Overlay the third image (if provided)
    if bgr_img is not None:
        # Resize the third image to 1.5 times its original size
        bgr_img_resized = cv2.resize(bgr_img, (int(bgr_img.shape[1] * 1.75), int(bgr_img.shape[0] * 1.75)))
        
        # Calculate the position to overlay the third image at the center
        x_offset3 = (width - bgr_img_resized.shape[1]) - 220
        y_offset3 = (height - bgr_img_resized.shape[0]) - 100
        
        # Overlay the resized third image onto the background
        background[y_offset3:y_offset3+bgr_img_resized.shape[0], x_offset3:x_offset3+bgr_img_resized.shape[1]] = bgr_img_resized

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

def read_img(ser, packet_size = 9600, image_size = 320 * 240 * 3):
    
    serial_buff = [105, 0]
    image_buff_index = 0
    img = bytearray()
    iterations = int(image_size / packet_size)

    for i in range(iterations):
        serial_buff[1] = image_buff_index
        ser.write(bytearray(serial_buff))
        data_bytes = ser.read(packet_size)
        img.extend(data_bytes)
        # print('\n', i, len(data_bytes))

        image_buff_index += 1


        if image_buff_index > 24:
            image_buff_index = 0

    return img


def main():

    choice = sys.argv[1]
    baud_rate = 115200  
    serial_port = '/dev/ttyACM0'
    window_name = 'DetectionResult'
    org = (165, 300) 

    # Open the serial port
    ser = serial.Serial(serial_port, baud_rate)

    # Create a blank white image
    width, height = 800, 600
    # background_image = create_background_image(width, height)

    img_size = 320 * 240 * 3
    packet_size = 9600

    while True:
        if choice == 'count':
            print(choice)
            # Write command to serial port
            ser.write(b'c')
            print(1)
            # Read data from the serial port
            data_bytes = ser.read(4)
            print(data_bytes)
            # Unpack bytes into uint32_t integer
            
            integer_value = struct.unpack('<I', data_bytes)[0]
            # print("Received integer:", integer_value)

            # Create background_image with logo
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
                
            # Handle keyboard interruption
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        elif choice == 'img':
            t0 = time.time()
            ser.flush()
            time.sleep(0.1)  # Adjust the delay as needed
            

            ser.write(b'c')
            # Read data from the serial port
            head_count_bytes = ser.read(4)
            head_count_int = struct.unpack('<I', head_count_bytes)[0]
            head_count_int -= 1

            # ser.write(b'b')
            # Read data from the serial port
            # b_box_bytes = ser.read(16)
            # b_box_int = struct.unpack('<4I', b_box_bytes)
            # print(b_box_int, end = ' ')


            img = read_img(ser, packet_size, img_size)

            rgb_image = np.frombuffer(img, dtype=np.uint8).reshape((240, 320, 3))
            bgr_image = cv2.cvtColor(rgb_image, cv2.COLOR_RGB2BGR)
            
            frame_image = create_background_image(width, height, bgr_img = bgr_image)

            data = 'HEAD COUNT'
            org = (590, 150) 
            frame_image = draw_text(frame_image, data, org, fontScale = 1)

            data = str(head_count_int)
            org = (620, 400) 
            frame_image = draw_text(frame_image, data, org, fontScale = 8)


                    # Display the image 
            cv2.namedWindow('RGB Image', cv2.WND_PROP_FULLSCREEN)
            cv2.setWindowProperty('RGB Image', cv2.WND_PROP_FULLSCREEN,cv2.WINDOW_FULLSCREEN)
            cv2.imshow('RGB Image', frame_image)

            t1 = time.time()

            diff = t1 - t0
            print(diff)
            
            k = cv2.waitKey(30)  # Add a slight delay to allow the window to update

            if k == 115:
                img_name = 'img' + str(t1) + '.jpg'
                cv2.imwrite(img_name, frame_image)
            if k == 27:
                cv2.destroyAllWindows()
                ser.close()
                break



    # Close the serial port
    ser.close()
    # Close OpenCV windows
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
