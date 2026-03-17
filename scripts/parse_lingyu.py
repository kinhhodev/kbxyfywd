import struct
import json
import os

def read_bytes_symm(data, offset, mapping):
    if offset + 4 > len(data):
        return [], offset, "Data too short for list length"
        
    length = struct.unpack('<i', data[offset:offset+4])[0]
    offset += 4
    
    results = []
    for i in range(length):
        if offset + 4 > len(data):
            return results, offset, f"Truncated at index {i} (symmId)"
        symm_id = struct.unpack('<i', data[offset:offset+4])[0]
        offset += 4
        
        if offset + 4 > len(data):
            return results, offset, f"Truncated at index {i} (symmIndex)"
        symm_index = struct.unpack('<i', data[offset:offset+4])[0]
        offset += 4
        
        if offset + 4 > len(data):
            return results, offset, f"Truncated at index {i} (symmFlag)"
        symm_flag = struct.unpack('<i', data[offset:offset+4])[0]
        offset += 4
        
        # User explicitly mentioned: "字符串。前 2 字节为长度（小端）"
        if offset + 2 > len(data):
            return results, offset, f"Truncated at index {i} (petName len)"
        name_len = struct.unpack('<H', data[offset:offset+2])[0]
        offset += 2
        
        if offset + name_len > len(data):
            return results, offset, f"Truncated at index {i} (petName content)"

        pet_name_bytes = data[offset:offset+name_len]
        try:
            pet_name = pet_name_bytes.decode('utf-8')
        except:
            pet_name = pet_name_bytes.decode('gbk', errors='replace')
        offset += name_len
        
        if offset + 4 > len(data):
            return results, offset, f"Truncated at index {i} (symmType)"
        symm_type = struct.unpack('<i', data[offset:offset+4])[0]
        offset += 4
        
        # sortValue would be looked up from XML here
        
        if offset + 4 > len(data):
            return results, offset, f"Truncated at index {i} (nativeLength)"
        native_len = struct.unpack('<i', data[offset:offset+4])[0]
        offset += 4
        
        native_list = []
        for j in range(native_len):
            if offset + 8 > len(data):
                break
            n_enum = struct.unpack('<i', data[offset:offset+4])[0]
            offset += 4
            n_value = struct.unpack('<i', data[offset:offset+4])[0]
            offset += 4
            
            native_list.append({
                "nativeEnum": n_enum,
                "nativeValue": n_value,
                "nativeName": mapping.get(n_enum, "未知")
            })
            
        results.append({
            "symmId": symm_id,
            "symmIndex": symm_index,
            "symmFlag": symm_flag,
            "petName": pet_name,
            "symmType": symm_type,
            "nativeList": native_list
        })
        
    return results, offset, None

def parse_lingyu():
    file_path = r"D:\卡布桌面版\灵玉.txt"
    output_path = r"D:\卡布桌面版\灵玉解析结果.json"
    
    # ... (Keep hex reading logic) ...
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        lines = content.splitlines()
        hex_parts = []
        for line in lines:
            if "→" in line:
                parts = line.split("→", 1)
                if len(parts) > 1:
                    part = parts[1].strip()
                    if "被调试" in part: continue
                    hex_parts.append(part)
            else:
                hex_parts.append(line.strip())
        raw_hex = "".join(hex_parts)
        hex_data = "".join(c for c in raw_hex if c in "0123456789abcdefABCDEF")

    try:
        data = bytes.fromhex(hex_data)
    except Exception as e:
        print(f"Error: {e}")
        return

    mapping = {25: "体力", 21: "攻击", 22: "防御", 23: "法术", 24: "抗性", 26: "速度", 11: "威力", 12: "PP"}
    
    offset = 0
    # Header 12 bytes
    header = data[offset:offset+12]
    offset += 12
    
    # backFlag (int)
    if offset + 4 > len(data): return
    back_flag = struct.unpack('<i', data[offset:offset+4])[0]
    offset += 4
    print(f"backFlag: {back_flag}")
    
    final_list = []
    
    if back_flag == 1:
        # Call twice
        list1, offset, err1 = read_bytes_symm(data, offset, mapping)
        if err1: print(f"List 1 Warning: {err1}")
        final_list.extend(list1)
        
        list2, offset, err2 = read_bytes_symm(data, offset, mapping)
        if err2: print(f"List 2 Warning: {err2}")
        final_list.extend(list2)
    elif back_flag in [2, 3]:
        # Call once
        list1, offset, err1 = read_bytes_symm(data, offset, mapping)
        if err1: print(f"List 1 Warning: {err1}")
        final_list.extend(list1)
    elif back_flag == 4:
        print("Error: 请求出错了 (backFlag 4)")
        
    # Save
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump({
            "backFlag": back_flag,
            "totalCount": len(final_list),
            "data": final_list
        }, f, ensure_ascii=False, indent=4)
    print(f"Parsed {len(final_list)} items total.")

if __name__ == "__main__":
    parse_lingyu()

if __name__ == "__main__":
    parse_lingyu()
