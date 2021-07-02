
import sys
import random
import cv2

def add_salt_pepper(image):
    rows, cols, x = image.shape
    print(rows, cols)

    number_of_pixel = random.randint(600, 10000)
    for i in range(number_of_pixel):
        x = random.randint(0, cols-1)
        y = random.randint(0, rows-1)

        image[y][x] = 255
    
    number_of_pixel = random.randint(600, 10000)
    for i in range(number_of_pixel):
        x = random.randint(0, cols-1)
        y = random.randint(0, rows-1)

        image[y][x] = 0

    return image

input_file = sys.argv[1]
output_file = sys.argv[2]

img = cv2.imread(input_file)
add_salt_pepper(img)

cv2.imwrite(output_file, add_salt_pepper(img))