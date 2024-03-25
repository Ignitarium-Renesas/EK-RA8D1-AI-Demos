import serial
import time
import numpy as np
import struct
import cv2



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
    count = 0
    img_count = 0
    baud_rate = 115200
    serial_port = '/dev/ttyACM0'
    
    img_size = 320 * 240 * 3
    packet_size = 9600
    ser = serial.Serial(serial_port, baud_rate)

    print('press s for saving image\n')

    # try:
    while True:
        ser.flush()
        time.sleep(0.1)  # Adjust the delay as needed

        t0 = time.time()

        img = read_img(ser, packet_size, img_size)

        t1 = time.time()

        diff = t1 - t0
        print(diff)
        rgb_image = np.frombuffer(img, dtype=np.uint8).reshape((240, 320, 3))
        bgr_image = cv2.cvtColor(rgb_image, cv2.COLOR_RGB2BGR)
        cv2.imshow('RGB Image', bgr_image)

        k = cv2.waitKey(30)  # Add a slight delay to allow the window to update

        if k == 115:
            img_name = 'img' + str(t1) + '.jpg'
            cv2.imwrite(img_name, bgr_image)
            count += 1

        if k == 27:
            cv2.destroyAllWindows()
            ser.close()
            break

if __name__ == "__main__":
    main()
