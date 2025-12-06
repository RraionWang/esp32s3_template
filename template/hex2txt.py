# hex_to_txt_18.py
# 将二进制文件转换为十六进制文本文件，每 18 字节换一行

def file_to_hex_txt_18(input_bin_path, output_txt_path):
    with open(input_bin_path, "rb") as f:
        data = f.read()

    lines = []
    for i in range(0, len(data), 18):
        chunk = data[i:i+18]
        line = " ".join(f"{b:02X}" for b in chunk)
        lines.append(line)

    with open(output_txt_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"转换成功！文件大小：{len(data)} 字节")
    print(f"共生成 {len(lines)} 行")
    print(f"输出：{output_txt_path}")


if __name__ == "__main__":
    input_file = "canon.mid"     # 输入文件
    output_file = "output.txt"   # 输出文本

    file_to_hex_txt_18(input_file, output_file)
