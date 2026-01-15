import os
import sys

BYTES_PER_ROW = 12

def tflite_to_header(source_file, dest_file, var_name="g_model"):
    if not os.path.isfile(source_file):
        print(f"[ERROR] File not found: {source_file}")
        return False
        
    with open(source_file, "rb") as fp:
        raw = fp.read()
    
    size = len(raw)
    print(f"[INFO] Processing {size} bytes from {source_file}")
    
    lines = ["#include <stdint.h>", "", f"const unsigned int {var_name}_len = {size};", f"alignas(8) const unsigned char {var_name}[] = {{"]
    
    chunks = [raw[i:i+BYTES_PER_ROW] for i in range(0, size, BYTES_PER_ROW)]
    for chunk in chunks:
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"  {hex_vals},")
    
    lines.append("};")
    
    with open(dest_file, "w") as fp:
        fp.write("\n".join(lines))
    
    print(f"[OK] Saved to {dest_file}")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python converter.py <input.tflite> <output.h> [variable_name]")
        sys.exit(1)
    
    src = sys.argv[1]
    dst = sys.argv[2]
    name = sys.argv[3] if len(sys.argv) > 3 else "g_model"
    tflite_to_header(src, dst, name)
