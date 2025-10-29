import cv2
import numpy as np

SPI_C = 1
SPI_Cpp = 2
LTDC = 3
SPI_MP = 4
GRAYSCALE_C = 5


def generate(filename, width, height, outputFileName, format):
    Img = cv2.imread(filename)
    Img = cv2.resize(Img, (width, height))

    if format == SPI_C:
        spi_c_generate(Img, outputFileName)
    elif format == SPI_Cpp:
        spi_cpp_generate(Img, outputFileName)
    elif format == SPI_MP:
        spi_mp_generate(Img, outputFileName)
    elif format == LTDC:
        ltdc_generate(Img, outputFileName)
    elif format == GRAYSCALE_C:
        grayscale_c_generate(Img, outputFileName)


def grayscale_c_generate(im, outputFileName):
    f = open(outputFileName + ".h", "w+")

    height, width, _ = im.shape
    
    # Convert image from BGR to Grayscale
    gray_image = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    
    # Flatten the 2D image array to a 1D array
    gray_image_flat = np.reshape(gray_image, (width * height))
    
    f.write("#include <stdint.h>\n\n")
    f.write("const uint8_t grayscale_img_data[%d] = {\n" % (width * height))

    for i in range(width * height):
        f.write("%s, " % hex(gray_image_flat[i]))
        if i != 0 and (i + 1) % 16 == 0:
            f.write("\n")

    f.write("\n};\n\n")
    
    f.write("/*\n")
    f.write("ImageTypeDef GRAYSCALE_IMG = {\n")
    f.write("    .pData = (uint8_t*)my_image_data,\n")
    f.write("    .width = %d,\n" % (width))
    f.write("    .height = %d,\n" % (height))
    f.write("    .size = %d,\n" % (width*height))
    f.write("    .format = 0 // Assuming 0 is for Grayscale\n")
    f.write("};\n*/\n\n")

    f.close()
    print("Grayscale C header file '%s.h' generated successfully." % outputFileName)    

def spi_c_generate(im, outputFileName):
    f = open(outputFileName + ".h", "w+")

    height, width, _ = im.shape
    img565 = cv2.cvtColor(im, cv2.COLOR_BGR2BGR565)
    img565 = img565.astype(np.uint8)

	# Byte swap -> Opencv and ili9341 are not compatiable in byte order.
    imgh = img565[:, :, 0].copy()
    img565[:, :, 0] = img565[:, :, 1].copy()
    img565[:, :, 1] = imgh.copy()

	# 2D to 1D conversion
    img565 = np.reshape(img565, (width*height*2))

    f.write("uint8_t RGB565_IMG_ARRAY[%d]={\n" % (width*height*2))

    for i in range(width*height*2):
        f.write("%s, " % hex(img565[i]))
        if i != 0 and i % 20 == 0:
            f.write("\n")

    f.write("};\n\n\n")
    f.write("ImageTypeDef RGB565_IMG = {\n")
    f.write(".pData = RGB565_IMG_ARRAY,\n")
    f.write(".width = %d,\n" % (width))
    f.write(".height = %d,\n" % (height))
    f.write(".size = %d,\n" % (width*height*2))
    f.write(".format = 1\n")
    f.write("};\n\n")

    f.close()


def spi_cpp_generate(im, outputFileName):
    f = open(outputFileName + ".hpp", "w+")

    height, width, _ = im.shape
    img565 = cv2.cvtColor(im, cv2.COLOR_BGR2BGR565)
    img565 = img565.astype(np.uint8)

    # Byte swap -> Opencv and ili9341 are not compatiable in byte order.
    imgh = img565[:, :, 0].copy()
    img565[:, :, 0] = img565[:, :, 1].copy()
    img565[:, :, 1] = imgh.copy()

	# 2D to 1D conversion
    img565 = np.reshape(img565, (width*height*2))

    f.write("#include \"image.hpp\"\n\n\n")
    f.write("uint8_t RGB565_IMG_ARRAY[%d]={\n" % (width*height*2))

    for i in range(width*height*2):
        f.write("%s, " % hex(img565[i]))
        if i != 0 and i % 20 == 0:
            f.write("\n")

    f.write("};\n\n\n")
    f.write("IMAGE RGB565_IMG;\n")

    f.close()


def spi_mp_generate(im, outputFileName):
    f = open(outputFileName + ".py", "w+")
    
    height, width, _ = im.shape
    img565 = cv2.cvtColor(im, cv2.COLOR_BGR2BGR565)
    img565 = img565.astype(np.uint8)
    
    # Byte swap -> Opencv and ili9341 are not compatiable in byte order.
    imgh = img565[:, :, 0].copy()
    img565[:, :, 0] = img565[:, :, 1].copy()
    img565[:, :, 1] = imgh.copy()
    
    # 2D to 1D conversion
    img565 = np.reshape(img565, (width*height*2))
    
    f.write("pData=[\n")
    
    for i in range(width*height*2):
        f.write("%s, " % hex(img565[i]))
        if i != 0 and i % 20 == 0:
            f.write("\n")
            
    f.write("]\n\n\n")
    f.write("width = %d\n" % (width))
    f.write("height = %d\n" % (height))
    f.write("size = %d\n" % (width*height*2))
    
    f.close()


def ltdc_generate(im, outputFileName):
    f = open(outputFileName + ".h", "w+")
    
    height, width, _ = im.shape
    img8888 = cv2.cvtColor(im, cv2.COLOR_BGR2BGRA)
    img8888 = img8888.astype(np.uint32)

	# 2D to 1D conversion
    img8888 = np.reshape(img8888, (width*height*4))
    
    f.write("uint8_t RGB8888_IMG_ARRAY[%d]={\n" % (width*height*4))

    for i in range(width*height*4):
        f.write("%s, " % hex(img8888[i]))
        if i != 0 and i % 20 == 0:
            f.write("\n")

    f.write("};\n")
    f.close()