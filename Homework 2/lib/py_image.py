import py_serialimg
import numpy as np
import cv2 

COM_PORT = "COM9" 

TEST_IMAGE_FILENAME = "monk.png"  

print(f"Seri port {COM_PORT} başlatılıyor...")
py_serialimg.SERIAL_Init(COM_PORT)
print("Port başlatıldı.")

mandrill = TEST_IMAGE_FILENAME

try:
    while 1:
        print("\nSTM32'den istek bekleniyor (PollForRequest)...")
        rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()

        if rqType == py_serialimg.MCU_WRITES:
            print(f"STM32 görüntü gönderiyor (Boyut: {width}x{height}). Alınıyor...")
            img = py_serialimg.SERIAL_IMG_Read()
            print("Görüntü başarıyla alındı.")
            
            received_filename = "received_from_f446re.png"
            cv2.imwrite(received_filename, img)
            print(f"Görüntü '{received_filename}' olarak kaydedildi.")

        elif rqType == py_serialimg.MCU_READS:
            print(f"STM32 görüntü istiyor (Boyut: {width}x{height}). Gönderiliyor...")
            img = py_serialimg.SERIAL_IMG_Write(mandrill)
            print(f"'{mandrill}' görüntüsü (128x128'e küçültülerek) başarıyla gönderildi.")

except KeyboardInterrupt:
    print("\nProgram durduruldu.")
except Exception as e:
    print(f"\nBir hata oluştu: {e}")
    print(f"COM portunun ('{COM_PORT}') doğru olduğundan ve STM32 kartının bağlı olduğundan emin olun.")