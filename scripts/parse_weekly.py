#!/usr/bin/env python3
"""解析weeklyactivity XML"""
import xml.etree.ElementTree as ET

# 解析XML
tree = ET.parse('D:/AItrace/CE/.trae/kbwebui/swf_cache/weeklyactivity_new.xml')
root = tree.getroot()

# 查找包含'红莓'、'采摘'、'Berry'的活动
print('查找采摘红莓果活动...')
print()

activities = []
for items in root.findall('items'):
    for item in items.findall('item'):
        name_elem = item.find('name')
        id_elem = item.find('id')
        url_elem = item.find('url')
        
        name = name_elem.text if name_elem is not None else ''
        act_id = id_elem.text if id_elem is not None else ''
        url = url_elem.text if url_elem is not None else ''
        
        if '红莓' in name or '采摘' in name or 'berry' in name.lower():
            print(f'找到活动: {name}')
            print(f'  ID: {act_id}')
            print(f'  URL: {url}')
            print()
            activities.append({'name': name, 'id': act_id, 'url': url})

# 列出最新的几个活动
print('\n最新活动列表（后30个）:')
all_activities = []
for items in root.findall('items'):
    for item in items.findall('item'):
        name_elem = item.find('name')
        id_elem = item.find('id')
        url_elem = item.find('url')
        desc_elem = item.find('desc')
        
        name = name_elem.text if name_elem is not None else ''
        act_id = id_elem.text if id_elem is not None else ''
        url = url_elem.text if url_elem is not None else ''
        desc = desc_elem.text if desc_elem is not None else ''
        
        all_activities.append({'name': name, 'id': act_id, 'url': url, 'desc': desc})

for act in all_activities[-30:]:
    url_short = act['url'][:60] if act['url'] else 'N/A'
    print(f"  {act['id']}: {act['name']} -> {url_short}")
