
import sys
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

from matplotlib import rcParams

image_A_path = sys.argv[1]
image_B_path = sys.argv[2]

rcParams['figure.figsize'] = 11 ,8

image_A = mpimg.imread(image_A_path)
image_B = mpimg.imread(image_B_path)

fig, ax = plt.subplots(1, 2)

ax[0].imshow(image_A)
ax[1].imshow(image_B)
plt.show()
